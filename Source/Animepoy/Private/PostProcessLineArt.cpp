#include "PostProcessLineArt.h"
#include "ShaderCompiler.h"
#include "SceneRendering.h"
#include "SceneTextureParameters.h"
#include "Substrate/Substrate.h"
#include "PixelShaderUtils.h"

namespace {
	class FDetectLineCS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FDetectLineCS);
		SHADER_USE_PARAMETER_STRUCT(FDetectLineCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)
			SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSubstrateGlobalUniformParameters, Substrate)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
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

namespace
{
	static TAutoConsoleVariable<int32> CVarSubstrate(
		TEXT("r.Substrate"),
		0,
		TEXT("Enable Substrate materials (Beta)."),
		ECVF_ReadOnly | ECVF_RenderThreadSafe);

	bool IsSubstrateEnabled()
	{
		return CVarSubstrate.GetValueOnAnyThread() > 0;
	}

	TRDGUniformBufferRef<FSubstrateGlobalUniformParameters> BindSubstrateGlobalUniformParameters(const FViewInfo& View)
	{
		check(View.SubstrateViewData.SubstrateGlobalUniformParameters != nullptr || !IsSubstrateEnabled());
		return View.SubstrateViewData.SubstrateGlobalUniformParameters;
	}
}

void AddLineArtPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FLineArtPassInputs& Inputs)
{
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport = FScreenPassTextureViewport(View.ViewRect);

	FScreenPassTexture SceneColor((*Inputs.SceneTextures)->SceneColorTexture, View.ViewRect);
	FScreenPassTexture SceneDepth((*Inputs.SceneTextures)->SceneDepthTexture, View.ViewRect);

	FRDGTextureRef LineTexture{};
	{
		RDG_EVENT_SCOPE(GraphBuilder, "PostProcessLineDetection");

		FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(Viewport.Extent, PF_R32_UINT, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV);
		LineTexture = GraphBuilder.CreateTexture(Desc, TEXT("LineTexture"));

		FRDGTextureUAVRef LineTextureUAV = GraphBuilder.CreateUAV(LineTexture);
		AddClearUAVPass(GraphBuilder, LineTextureUAV, (uint32)0);

		FDetectLineCS::FParameters* Parameters = GraphBuilder.AllocParameters<FDetectLineCS::FParameters>();
		Parameters->View = View.ViewUniformBuffer;
		Parameters->SceneTextures = GetSceneTextureShaderParameters(Inputs.SceneTextures);
		Parameters->Substrate = BindSubstrateGlobalUniformParameters(View);
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
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

	if (Inputs.bPreview)
	{
		RDG_EVENT_SCOPE(GraphBuilder, "PostProcessLineComposite Preview");

		FClearSceneColorAndGBufferPS::FParameters* Parameters = GraphBuilder.AllocParameters<FClearSceneColorAndGBufferPS::FParameters>();
		Parameters->RenderTargets[0] = FRenderTargetBinding(SceneColor.Texture, ERenderTargetLoadAction::ENoAction);

		FRHIBlendState* BlendState = TStaticBlendState<
			CW_RGB, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero,
			CW_ALPHA
		>::GetRHI();

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			ShaderMap,
			RDG_EVENT_NAME("ClearSceneColorPS"),
			TShaderMapRef<FClearSceneColorAndGBufferPS>(ShaderMap),
			Parameters,
			Viewport.Rect,
			BlendState);
	}

	{
		RDG_EVENT_SCOPE(GraphBuilder, "PostProcessLineComposite");

		FCompositeLinePS::FParameters* Parameters = GraphBuilder.AllocParameters<FCompositeLinePS::FParameters>();
		Parameters->View = View.ViewUniformBuffer;
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->LineTexture = LineTexture;
		Parameters->LineColor = Inputs.LineColor;
		Parameters->LineWidth = Inputs.LineWidth * Inputs.LineWidth;
		Parameters->SearchRangeMin = -(Inputs.LineWidth / 2);
		Parameters->SearchRangeMax = (Inputs.LineWidth - 1) / 2;
		Parameters->RenderTargets[0] = FRenderTargetBinding(SceneColor.Texture, ERenderTargetLoadAction::ELoad);
		Parameters->RenderTargets.DepthStencil = FDepthStencilBinding(SceneDepth.Texture, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilNop);

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