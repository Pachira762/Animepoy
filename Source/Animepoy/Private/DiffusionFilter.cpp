#include "DiffusionFilter.h"
#include "PostProcess/PostProcessDownsample.h"
#include "PostProcess/PostProcessWeightedSampleSum.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompiler.h"
#include "SceneRendering.h"
#include "SceneTextureParameters.h"
#include "PixelShaderUtils.h"
#include "UnrealEngine.h"

namespace {
	const int32 GTileSizeX = 8;
	const int32 GTileSizeY = 8;
	const int32 GDownsampleFactor = 4;

	class FGenerateMaskCS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FGenerateMaskCS);
		SHADER_USE_PARAMETER_STRUCT(FGenerateMaskCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER(float, LuminanceMin)
			SHADER_PARAMETER(float, InvLuminanceWidth)
			SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutMaskTexture)
		END_SHADER_PARAMETER_STRUCT()

			static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
			OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GDownsampleFactor);
		}
	};

	IMPLEMENT_GLOBAL_SHADER(FGenerateMaskCS, "/AnimepoyShaders/Private/DiffusionFilter.usf", "GenerateMaskCS", SF_Compute);

	class FCompositePS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FCompositePS);
		SHADER_USE_PARAMETER_STRUCT(FCompositePS, FGlobalShader);

		class FBlendMode : SHADER_PERMUTATION_ENUM_CLASS("BLEND_MODE", EDiffusionFilterBlendMode);
		using FPermutationDomain = TShaderPermutationDomain<FBlendMode>;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BlurredColorTexture)
			SHADER_PARAMETER(float, BlendAmount)
			RENDER_TARGET_BINDING_SLOTS()
			END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FCompositePS, "/AnimepoyShaders/Private/DiffusionFilter.usf", "CompositePS", SF_Pixel);
}

FScreenPassTexture AddDiffusionFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, FDiffusionFilterInputs& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "DiffusionFilter");

	FRDGTextureRef MaskTexture{};
	{
		FIntPoint MaskTextureExtent = FIntPoint::DivideAndRoundUp(Inputs.SceneColor.ViewRect.Size(), GDownsampleFactor);

		FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(MaskTextureExtent, PF_B8G8R8A8, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
		MaskTexture = GraphBuilder.CreateTexture(Desc, TEXT("DiffusionFilterMask"));

		FGenerateMaskCS::FParameters* Parameters = GraphBuilder.AllocParameters<FGenerateMaskCS::FParameters>();
		Parameters->Input = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(Inputs.SceneColor));
		Parameters->Output = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(MaskTextureExtent));
		Parameters->SceneColorTexture = Inputs.SceneColor.Texture;
		Parameters->LuminanceMin = FMath::Clamp(Inputs.LuminanceMin, 0.0, 1.0);
		Parameters->InvLuminanceWidth = 1.f / FMath::Max(Inputs.LuminanceMax - Inputs.LuminanceMin, 0.00001f);
		Parameters->OutMaskTexture = GraphBuilder.CreateUAV(MaskTexture);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("DiffusionFilterGenerateMask"),
			TShaderMapRef<FGenerateMaskCS>(View.ShaderMap),
			Parameters,
			FComputeShaderUtils::GetGroupCount(MaskTextureExtent, FIntPoint(GTileSizeX, GTileSizeY)));
	}

	FRDGTextureRef BlurredColorTexture{};
	{
		FGaussianBlurInputs BlurInputs;
		BlurInputs.NameX = TEXT("DiffusionFilterMaskBlurX");
		BlurInputs.NameY = TEXT("DiffusionFilterMaskBlurY");
		BlurInputs.Filter = FScreenPassTextureSlice::CreateFromScreenPassTexture(GraphBuilder, FScreenPassTexture(MaskTexture));
		BlurInputs.TintColor = FLinearColor::White;
		BlurInputs.CrossCenterWeight = FVector2f::ZeroVector;
		BlurInputs.KernelSizePercent = Inputs.BlurPercentage;
		BlurInputs.UseMirrorAddressMode = true;

		BlurredColorTexture = AddGaussianBlurPass(GraphBuilder, View, BlurInputs).Texture;
	}

	FScreenPassRenderTarget Output = Inputs.OverrideOutput;
	{
		if (!Output.IsValid())
		{
			Output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, Inputs.SceneColor, View.GetOverwriteLoadAction(), TEXT("DiffusionFilter"));
		}

		FCompositePS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FCompositePS::FBlendMode>(Inputs.bPreviewMask ? EDiffusionFilterBlendMode::Preview : Inputs.BlendMode);

		FCompositePS::FParameters* Parameters = GraphBuilder.AllocParameters<FCompositePS::FParameters>();
		Parameters->View = View.ViewUniformBuffer;
		Parameters->Input = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(Inputs.SceneColor));
		Parameters->Output = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(Output));
		Parameters->SceneColorTexture = Inputs.SceneColor.Texture;
		Parameters->BlurredColorTexture = BlurredColorTexture;
		Parameters->BlendAmount = Inputs.bPreviewMask ? 1.f : FMath::Clamp(Inputs.Intensity, 0.f, 1.f);
		Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			View.ShaderMap,
			RDG_EVENT_NAME("DiffusionFilterComposite"),
			TShaderMapRef<FCompositePS>(View.ShaderMap, PermutationVector),
			Parameters,
			Output.ViewRect,
			TStaticBlendState<CW_RGB>::GetRHI());
	}

	return MoveTemp(Output);
}
