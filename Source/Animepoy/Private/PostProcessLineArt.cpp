#include "PostProcessLineArt.h"
#include "ShaderCompiler.h"
#include "SceneRendering.h"
#include "SceneTextureParameters.h"
#include "PixelShaderUtils.h"

namespace {
	class FDetectLineCS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FDetectLineCS);
		SHADER_USE_PARAMETER_STRUCT(FDetectLineCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferATexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GBufferBTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
			SHADER_PARAMETER(float, MaterialThreshold)
			SHADER_PARAMETER(float, DepthThreshold)
			SHADER_PARAMETER(float, NormalThreshold)
			SHADER_PARAMETER(float, PlanarThreshold)
			SHADER_PARAMETER(float, NonLineSpecular)
			SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutLineTexture)
			END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FDetectLineCS, "/AnimepoyShaders/Private/PostProcessLineArt.usf", "DetectLineCS", SF_Compute);

	class FCompositeLinePS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FCompositeLinePS);
		SHADER_USE_PARAMETER_STRUCT(FCompositeLinePS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, LineTexture)
			SHADER_PARAMETER(FVector4f, LineColor)
			SHADER_PARAMETER(int, LineWidth)
			SHADER_PARAMETER(int, SearchRangeMin)
			SHADER_PARAMETER(int, SearchRangeMax)
			RENDER_TARGET_BINDING_SLOTS()
			END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FCompositeLinePS, "/AnimepoyShaders/Private/PostProcessLineArt.usf", "CompositeLinePS", SF_Pixel);

	class FClearSceneColorAndGBufferPS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FClearSceneColorAndGBufferPS);
		SHADER_USE_PARAMETER_STRUCT(FClearSceneColorAndGBufferPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			RENDER_TARGET_BINDING_SLOTS()
			END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FClearSceneColorAndGBufferPS, "/AnimepoyShaders/Private/PostProcessLineArt.usf", "ClearSceneColorAndGBufferPS", SF_Pixel);
}

FRDGTextureRef AddDetectLinePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FDetectLinePassInputs& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "PostProcessLineDetection");

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport = FScreenPassTextureViewport(View.ViewRect);

	FRDGTextureRef LineTexture{};
	{
		FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(Viewport.Extent, PF_R32_UINT, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
		LineTexture = GraphBuilder.CreateTexture(Desc, TEXT("LineTexture"));

		FRDGTextureUAVRef LineTextureUAV = GraphBuilder.CreateUAV(LineTexture);
		AddClearUAVPass(GraphBuilder, LineTextureUAV, (uint32)0);

		FDetectLineCS::FParameters* Parameters = GraphBuilder.AllocParameters<FDetectLineCS::FParameters>();
		Parameters->View = View.ViewUniformBuffer;
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->GBufferATexture = Inputs.GBufferA;
		Parameters->GBufferBTexture = Inputs.GBufferB;
		Parameters->SceneDepthTexture = Inputs.SceneDepth;
		Parameters->MaterialThreshold = FMath::Lerp(2.f, 0.f, Inputs.MaterialLineIntensity);
		Parameters->DepthThreshold = FMath::Lerp(Inputs.DepthLineIntensity == 0.f ? 1.f : 0.01f, 0.f, Inputs.DepthLineIntensity);
		Parameters->NormalThreshold = FMath::Lerp(-1.f, 1.f, Inputs.NormalLineIntensity);
		Parameters->PlanarThreshold = FMath::Lerp(1.f, 0.f, Inputs.PlanarLineIntensity);
		Parameters->NonLineSpecular = static_cast<int>(255.f * Inputs.NonLineSpecular) / 255.f;
		Parameters->OutLineTexture = LineTextureUAV;

		TShaderMapRef<FDetectLineCS> ComputeShader(ShaderMap);
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("DetectLineCS"),
			ComputeShader,
			Parameters,
			FComputeShaderUtils::GetGroupCount(Viewport.Rect.Size(), FIntPoint(8, 8)));
	}

	return LineTexture;
}

void AddCompositeLinePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FCompositeLinePassInputs& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "PostProcessLineComposite");

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport = FScreenPassTextureViewport(View.ViewRect);

	if (Inputs.bPreview && Inputs.GBufferB)
	{
		FClearSceneColorAndGBufferPS::FParameters* Parameters = GraphBuilder.AllocParameters<FClearSceneColorAndGBufferPS::FParameters>();
		Parameters->RenderTargets[0] = FRenderTargetBinding(Inputs.Target, ERenderTargetLoadAction::ENoAction);
		Parameters->RenderTargets[1] = FRenderTargetBinding(Inputs.GBufferB, ERenderTargetLoadAction::ENoAction);

		FRHIBlendState* BlendState = TStaticBlendState<
			CW_RGB, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero,
			CW_ALPHA
		>::GetRHI();

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			ShaderMap,
			RDG_EVENT_NAME("ClearSceneColorAndGBufferPS"),
			TShaderMapRef<FClearSceneColorAndGBufferPS>(ShaderMap),
			Parameters,
			Viewport.Rect,
			BlendState);
	}

	{
		FCompositeLinePS::FParameters* Parameters = GraphBuilder.AllocParameters<FCompositeLinePS::FParameters>();
		Parameters->View = View.ViewUniformBuffer;
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->LineTexture = Inputs.LineTexture;
		Parameters->LineColor = Inputs.LineColor;
		Parameters->LineWidth = Inputs.LineWidth * Inputs.LineWidth;
		Parameters->SearchRangeMin = -(Inputs.LineWidth / 2);
		Parameters->SearchRangeMax = (Inputs.LineWidth - 1) / 2;
		Parameters->RenderTargets[0] = FRenderTargetBinding(Inputs.Target, ERenderTargetLoadAction::ELoad);
		Parameters->RenderTargets.DepthStencil = FDepthStencilBinding(Inputs.SceneDepth, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilNop);

		FRHIBlendState* BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_DestColor, BF_InverseSourceAlpha>::GetRHI();
		FRHIDepthStencilState* DepthStencilState = TStaticDepthStencilState<true, CF_Always>::GetRHI();

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			ShaderMap,
			RDG_EVENT_NAME("CompositeLinePS"),
			TShaderMapRef<FCompositeLinePS>(ShaderMap),
			Parameters,
			Viewport.Rect,
			BlendState,
			nullptr,
			DepthStencilState);
	}
}
