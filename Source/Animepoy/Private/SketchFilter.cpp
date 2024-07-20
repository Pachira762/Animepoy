// @Custom
#include "SketchFilter.h"
#include "PostProcess/PostProcessDownsample.h"
#include "PostProcess/PostProcessWeightedSampleSum.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"
#include "SceneRendering.h"
#include "SceneTextureParameters.h"
#include "PixelShaderUtils.h"
#include "RenderGraphUtils.h"
#include "UnrealEngine.h"

namespace {
	class FSketchFilterSetupCS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FSketchFilterSetupCS);
		SHADER_USE_PARAMETER_STRUCT(FSketchFilterSetupCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER(int32, FilterSize)
			SHADER_PARAMETER(int32, FilterType)
			SHADER_PARAMETER(int32, FilterTarget)
			SHADER_PARAMETER(int32, DebugFilter)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BaseColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutColorAndLuminanceTexture)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
		}
	};

	IMPLEMENT_GLOBAL_SHADER(FSketchFilterSetupCS, "/AnimepoyShaders/Private/SketchFilter.usf", "SketchFilterSetupCS", SF_Compute);

	class FSketchFilterPS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FSketchFilterPS);
		SHADER_USE_PARAMETER_STRUCT(FSketchFilterPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER(int32, FilterSize)
			SHADER_PARAMETER(int32, FilterType)
			SHADER_PARAMETER(int32, FilterTarget)
			SHADER_PARAMETER(int32, DebugFilter)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ColorAndLuminanceTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BaseColorTexture)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FSketchFilterPS, "/AnimepoyShaders/Private/SketchFilter.usf", "SketchFilterPS", SF_Pixel);
}

void AddSketchFilterSetupPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInputs& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "KuwaharaFilter");
}

void AddSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInputs& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "SketchFilter");

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport(View.ViewRect);

	FRDGTextureRef SceneColorTexture = (*Inputs.SceneTextures)->SceneColorTexture;
	FRDGTextureRef BaseColorTexture = (*Inputs.SceneTextures)->GBufferCTexture;
	FRDGTextureRef ColorAndLuminanceTexture{};

	// Setup Pass
	{
		FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(SceneColorTexture->Desc.Extent, SceneColorTexture->Desc.Format, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
		ColorAndLuminanceTexture = GraphBuilder.CreateTexture(Desc, TEXT("ColorAndLuminanceTexture"));

		FSketchFilterSetupCS::FParameters* Parameters = GraphBuilder.AllocParameters<FSketchFilterSetupCS::FParameters>();
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->FilterSize = Inputs.FilterSize;
		Parameters->FilterType = static_cast<int32>(Inputs.FilterType);
		Parameters->FilterTarget = static_cast<int32>(Inputs.FilterTarget);
		Parameters->DebugFilter = static_cast<int32>(Inputs.bDebugFilter);
		Parameters->SceneColorTexture = SceneColorTexture;
		Parameters->BaseColorTexture = BaseColorTexture;
		Parameters->OutColorAndLuminanceTexture = GraphBuilder.CreateUAV(ColorAndLuminanceTexture);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("SketchFilterSetupCS"),
			TShaderMapRef<FSketchFilterSetupCS>(ShaderMap),
			Parameters,
			FComputeShaderUtils::GetGroupCount(Viewport.Rect.Size(), FIntPoint(8, 8))
		);
	}

	// Draw Pass
	{
		FSketchFilterPS::FParameters* Parameters = GraphBuilder.AllocParameters<FSketchFilterPS::FParameters>();
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->FilterSize = Inputs.FilterSize;
		Parameters->FilterType = static_cast<int32>(Inputs.FilterType);
		Parameters->FilterTarget = static_cast<int32>(Inputs.FilterTarget);
		Parameters->DebugFilter = static_cast<int32>(Inputs.bDebugFilter);
		Parameters->ColorAndLuminanceTexture = ColorAndLuminanceTexture;
		Parameters->BaseColorTexture = BaseColorTexture;
		Parameters->RenderTargets[0] = FRenderTargetBinding(SceneColorTexture, ERenderTargetLoadAction::ELoad);

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			ShaderMap,
			RDG_EVENT_NAME("SketchFilterPS"),
			TShaderMapRef<FSketchFilterPS>(ShaderMap),
			Parameters,
			Viewport.Rect,
			TStaticBlendState<CW_RGB>::GetRHI());
	}
}