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
	class FGBufferSketchFilterPS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FGBufferSketchFilterPS);
		SHADER_USE_PARAMETER_STRUCT(FGBufferSketchFilterPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER(int32, FilterSize)
			SHADER_PARAMETER(int32, FilterType)
			SHADER_PARAMETER(int32, FilterTarget)
			SHADER_PARAMETER(int32, DebugFilter)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, OrientationTexture)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FGBufferSketchFilterPS, "/AnimepoyShaders/Private/SketchFilter.usf", "GBufferSketchFilterPS", SF_Pixel);

	class FSceneColorSketchFilterPS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FSceneColorSketchFilterPS);
		SHADER_USE_PARAMETER_STRUCT(FSceneColorSketchFilterPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER(int32, FilterSize)
			SHADER_PARAMETER(int32, FilterType)
			SHADER_PARAMETER(int32, FilterTarget)
			SHADER_PARAMETER(int32, DebugFilter)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, OrientationTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WorldNormalTexture)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FSceneColorSketchFilterPS, "/AnimepoyShaders/Private/SketchFilter.usf", "SceneColorSketchFilterPS", SF_Pixel);
}

namespace
{
	FRDGTextureRef CopyTexture(FRDGBuilder& GraphBuilder, FRDGTextureRef Texture, const TCHAR* Name)
	{
		FRDGTextureRef CopiedTexture = GraphBuilder.CreateTexture(Texture->Desc, Name);
		AddCopyTexturePass(GraphBuilder, Texture, CopiedTexture);
		return CopiedTexture;
	}
}

void AddGBufferSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FGBufferSketchFilterInputs& Inputs)
{
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport(View.ViewRect);

	Inputs.RenderTargets.Output[0];
	FRDGTextureRef SceneDepthTexture = (*Inputs.SceneTextures)->SceneDepthTexture;
	FRDGTextureRef WorldNormalTexture = Inputs.RenderTargets.Output[1].GetTexture(); // GBufferA
	FRDGTextureRef BaseColorTexture = Inputs.RenderTargets.Output[3].GetTexture(); // GBufferC
	FRDGTextureRef InputTexture{};
	FRDGTextureRef OrientationTexture{};
	FRDGTextureRef OutputTexture{};

	if (Inputs.bProcessBaseColor)
	{
		OutputTexture = BaseColorTexture;
		InputTexture = CopyTexture(GraphBuilder, BaseColorTexture, TEXT("CopiedBaseColor"));
		OrientationTexture = InputTexture;
	}
	else if (Inputs.bProcessWorldNormal)
	{
		OutputTexture = WorldNormalTexture;
		InputTexture = CopyTexture(GraphBuilder, WorldNormalTexture, TEXT("CopiedWorldNormal"));
		OrientationTexture = InputTexture;
	}
	check(InputTexture);

	FGBufferSketchFilterPS::FParameters* Parameters = GraphBuilder.AllocParameters<FGBufferSketchFilterPS::FParameters>();
	Parameters->View = View.ViewUniformBuffer;
	Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
	Parameters->FilterSize = Inputs.FilterSize;
	Parameters->FilterType = static_cast<int32>(Inputs.FilterType);
	Parameters->FilterTarget = static_cast<int32>(0);
	Parameters->DebugFilter = static_cast<int32>(Inputs.bDebugFilter);
	Parameters->InputTexture = InputTexture;
	Parameters->OrientationTexture = OrientationTexture;
	Parameters->RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::ELoad);

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		ShaderMap,
		RDG_EVENT_NAME("GBufferSketchFilterPS"),
		TShaderMapRef<FGBufferSketchFilterPS>(ShaderMap),
		Parameters,
		Viewport.Rect,
		TStaticBlendState<CW_RGB>::GetRHI());

}

void AddSceneColorSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FPostProcessSketchFilterInputs& Inputs)
{
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport(View.ViewRect);

	FRDGTextureRef SceneColorTexture = (*Inputs.SceneTextures)->SceneColorTexture; 
	FRDGTextureRef SceneDepthTexture = (*Inputs.SceneTextures)->SceneDepthTexture;
	FRDGTextureRef WorldNormalTexture = (*Inputs.SceneTextures)->GBufferATexture;
	FRDGTextureRef BaseColorTexture = (*Inputs.SceneTextures)->GBufferCTexture;

	FRDGTextureRef SrcSceneColorTexture = GraphBuilder.CreateTexture(SceneColorTexture->Desc, TEXT("SrcSceneColorTexture"));
	AddCopyTexturePass(GraphBuilder, SceneColorTexture, SrcSceneColorTexture);

	if (Inputs.FilterDirection == ESketchFilterDirection::None)
	{
		if (Inputs.FilterType == ESketchFilterType::Kuwahara)
		{
			FSceneColorSketchFilterPS::FParameters* Parameters = GraphBuilder.AllocParameters<FSceneColorSketchFilterPS::FParameters>();
			Parameters->View = View.ViewUniformBuffer;
			Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
			Parameters->FilterSize = Inputs.FilterSize;
			Parameters->FilterType = static_cast<int32>(Inputs.FilterType);
			Parameters->FilterTarget = static_cast<int32>(0);
			Parameters->DebugFilter = static_cast<int32>(Inputs.bDebugFilter);
			Parameters->InputTexture = SrcSceneColorTexture;
			Parameters->OrientationTexture = BaseColorTexture;
			Parameters->SceneColorTexture = SrcSceneColorTexture;
			Parameters->SceneDepthTexture = SceneDepthTexture;
			Parameters->WorldNormalTexture = (*Inputs.SceneTextures)->GBufferATexture;
			Parameters->RenderTargets[0] = FRenderTargetBinding(SceneColorTexture, ERenderTargetLoadAction::ELoad);

			FPixelShaderUtils::AddFullscreenPass(
				GraphBuilder,
				ShaderMap,
				RDG_EVENT_NAME("SceneColorSketchFilterPS"),
				TShaderMapRef<FSceneColorSketchFilterPS>(ShaderMap),
				Parameters,
				Viewport.Rect,
				TStaticBlendState<CW_RGB>::GetRHI());
		}
	}
}
