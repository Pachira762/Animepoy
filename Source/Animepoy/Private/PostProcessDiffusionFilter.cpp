#include "PostProcessDiffusionFilter.h"
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

		class FPreTonemapLuminance : SHADER_PERMUTATION_BOOL("USE_PRE_TONEMAP_LUMINANCE");
		using FPermutationDomain = TShaderPermutationDomain<FPreTonemapLuminance>;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, PreTonemap)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, PreTonemapColorTexture)
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

	IMPLEMENT_GLOBAL_SHADER(FGenerateMaskCS, "/AnimepoyShaders/Private/PostProcessDiffusionFilter.usf", "GenerateMaskCS", SF_Compute);

	class FCompositePS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FCompositePS);
		SHADER_USE_PARAMETER_STRUCT(FCompositePS, FGlobalShader);

		enum class EBlendMode : uint8
		{
			Lighten,
			Screen,
			Overlay,
			SoftLight,
			Debug,
			MAX
		};

		class FBlendMode : SHADER_PERMUTATION_ENUM_CLASS("DIFFUSION_BLEND_MODE", EBlendMode);
		using FPermutationDomain = TShaderPermutationDomain<FBlendMode>;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BlurredColorTexture)
			SHADER_PARAMETER(float, BlendAmount)
			RENDER_TARGET_BINDING_SLOTS()
			END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FCompositePS, "/AnimepoyShaders/Private/PostProcessDiffusionFilter.usf", "CompositePS", SF_Pixel);
}

FScreenPassTexture AddPostProcessDiffusionPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPostProcessDiffusionInputs& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "AnimeDiffusionFilter");

	FRDGTextureRef MaskTexture;
	{
		FIntPoint MaskTextureExtent = FIntPoint::DivideAndRoundUp(Inputs.SceneColor.ViewRect.Size(), GDownsampleFactor);

		FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(MaskTextureExtent, PF_B8G8R8A8, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
		MaskTexture = GraphBuilder.CreateTexture(Desc, TEXT("DiffusionMask"));

		FGenerateMaskCS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FGenerateMaskCS::FPreTonemapLuminance>(Inputs.bPreTonemapLuminance);

		FGenerateMaskCS::FParameters* Parameters = GraphBuilder.AllocParameters<FGenerateMaskCS::FParameters>();
		Parameters->Input = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(Inputs.SceneColor));
		Parameters->PreTonemap = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(View.ViewRect));
		Parameters->Output = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(MaskTextureExtent));
		Parameters->OutMaskTexture = GraphBuilder.CreateUAV(MaskTexture);
		Parameters->SceneColorTexture = Inputs.SceneColor.Texture;
		Parameters->PreTonemapColorTexture = Inputs.PreTonemapColor;
		Parameters->LuminanceMin = FMath::Clamp(Inputs.LuminanceMin, 0.0, 1.0);
		Parameters->InvLuminanceWidth = 1.f / FMath::Max(Inputs.LuminanceMax - Inputs.LuminanceMin, 0.00001f);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("DiffusionGenerateMask"),
			TShaderMapRef<FGenerateMaskCS>(View.ShaderMap, PermutationVector),
			Parameters,
			FComputeShaderUtils::GetGroupCount(MaskTextureExtent, FIntPoint(GTileSizeX, GTileSizeY)));
	}

	FRDGTextureRef BlurredColorTexture;
	{
		FGaussianBlurInputs BlurInputs;
		BlurInputs.NameX = TEXT("DiffusionBlurX");
		BlurInputs.NameY = TEXT("DiffusionBlurY");
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
			Output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, Inputs.SceneColor, View.GetOverwriteLoadAction(), TEXT("Diffusion"));
		}

		FCompositePS::EBlendMode BlendMode = (FCompositePS::EBlendMode)FMath::Clamp(static_cast<uint8>(Inputs.BlendMode), 0, (uint8)FCompositePS::EBlendMode::MAX - 1);

		FCompositePS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FCompositePS::FBlendMode>(Inputs.bDebugMask ? FCompositePS::EBlendMode::Debug : BlendMode);

		FCompositePS::FParameters* Parameters = GraphBuilder.AllocParameters<FCompositePS::FParameters>();
		Parameters->Input = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(Inputs.SceneColor));
		Parameters->Output = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(Output));
		Parameters->SceneColorTexture = Inputs.SceneColor.Texture;
		Parameters->BlurredColorTexture = BlurredColorTexture;
		Parameters->BlendAmount = Inputs.bDebugMask ? 1.f : FMath::Clamp(Inputs.Intensity, 0.f, 1.f);
		Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			View.ShaderMap,
			RDG_EVENT_NAME("Diffusion Composite"),
			TShaderMapRef<FCompositePS>(View.ShaderMap, PermutationVector),
			Parameters,
			Output.ViewRect,
			TStaticBlendState<CW_RGB>::GetRHI());
	}

	return MoveTemp(Output);
}
