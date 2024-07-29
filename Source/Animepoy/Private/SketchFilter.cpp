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
	class FSceneColorKuwaharaFilterPS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FSceneColorKuwaharaFilterPS);
		SHADER_USE_PARAMETER_STRUCT(FSceneColorKuwaharaFilterPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER(int32, FilterSize)
			SHADER_PARAMETER(int32, FilterType)
			SHADER_PARAMETER(int32, FilterTarget)
			SHADER_PARAMETER(int32, DebugFilter)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FSceneColorKuwaharaFilterPS, "/AnimepoyShaders/Private/SketchFilter.usf", "SceneColorKuwaharaFilterPS", SF_Pixel);
}

void AddGBufferSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInputs& Inputs)
{
}

void AddSceneColorSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInputs& Inputs)
{
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport(View.ViewRect);

	FRDGTextureRef SceneColorTexture = (*Inputs.SceneTextures)->SceneColorTexture; 
	FRDGTextureRef SceneDepthTexture = (*Inputs.SceneTextures)->SceneDepthTexture;

	FRDGTextureRef SrcSceneColorTexture = GraphBuilder.CreateTexture(SceneColorTexture->Desc, TEXT("SrcSceneColorTexture"));
	AddCopyTexturePass(GraphBuilder, SceneColorTexture, SrcSceneColorTexture);

	if (Inputs.FilterDirection == ESketchFilterDirection::None)
	{
		if (Inputs.FilterType == ESketchFilterType::Kuwahara)
		{
			FSceneColorKuwaharaFilterPS::FParameters* Parameters = GraphBuilder.AllocParameters<FSceneColorKuwaharaFilterPS::FParameters>();
			Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
			Parameters->FilterSize = Inputs.FilterSize;
			Parameters->FilterType = static_cast<int32>(Inputs.FilterType);
			Parameters->FilterTarget = static_cast<int32>(0);
			Parameters->DebugFilter = static_cast<int32>(Inputs.bDebugFilter);
			Parameters->SceneColorTexture = SrcSceneColorTexture;
			Parameters->SceneDepthTexture = SceneDepthTexture;
			Parameters->RenderTargets[0] = FRenderTargetBinding(SceneColorTexture, ERenderTargetLoadAction::ELoad);

			FPixelShaderUtils::AddFullscreenPass(
				GraphBuilder,
				ShaderMap,
				RDG_EVENT_NAME("SceneColorKuwaharaFilterPS"),
				TShaderMapRef<FSceneColorKuwaharaFilterPS>(ShaderMap),
				Parameters,
				Viewport.Rect,
				TStaticBlendState<CW_RGB>::GetRHI());
		}
	}
}
