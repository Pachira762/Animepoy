// Fill out your copyright notice in the Description page of Project AnimepoyRenderProxy.


#include "AnimepoySceneViewExtension.h"
#include "ShaderCompilerCore.h"
#include "ScreenPass.h"
#include "SceneTexturesConfig.h"
#include "SceneTextureParameters.h"
#include "ShaderParameterUtils.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "AnimepoySubsystem.h"
#include "Animepoy.h"
#include "PostProcessLineArt.h"
#include "PostProcessKuwaharaFilter.h"
#include "PostProcessDiffusionFilter.h"

FAnimepoySceneViewExtension::FAnimepoySceneViewExtension(const FAutoRegister& AutoRegister, UAnimepoySubsystem* WorldSubsystem)
	: FSceneViewExtensionBase(AutoRegister)
	, WorldSubsystem(WorldSubsystem)
{
}

void FAnimepoySceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	AnimepoyRenderProxy = WorldSubsystem->GetAnimepoyRenderProxy();
	bEnable = InView.Family->Scene->GetWorld() == WorldSubsystem->GetWorld() && AnimepoyRenderProxy.bEnable;
}

#if USE_POST_DEFERRED_LIGHTING_PASS
void FAnimepoySceneViewExtension::PostDeferredLighting_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	check(InView.bIsViewInfo);
	auto& View = static_cast<const FViewInfo&>(InView);

	if (ShouldProcessThisView() && AnimepoyRenderProxy.bLineArt)
	{
		FLineArtPassInputs PassInputs;
		PassInputs.SceneTextures = SceneTextures;
		PassInputs.DepthLineIntensity = AnimepoyRenderProxy.DepthLineIntensity;
		PassInputs.NormalLineIntensity = AnimepoyRenderProxy.NormalLineIntensity;
		PassInputs.PlanarLineIntensity = AnimepoyRenderProxy.PlanarLineIntensity;
		PassInputs.MaterialLineIntensity = AnimepoyRenderProxy.MaterialLineIntensity;
		PassInputs.LineWidth = AnimepoyRenderProxy.LineWidth;
		PassInputs.LineColor = AnimepoyRenderProxy.LineColor;
		PassInputs.bPreview = AnimepoyRenderProxy.bPreviewLine;

		AddLineArtPass(GraphBuilder, View, PassInputs);
	}
}
#endif // USE_POST_DEFERRED_LIGHTING_PASS

void FAnimepoySceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs)
{
	check(InView.bIsViewInfo);
	auto& View = static_cast<const FViewInfo&>(InView);

	if (ShouldProcessThisView() && AnimepoyRenderProxy.bPrePostProcessKuwaharaFilter)
	{
		FKuwaharaFilterInputs PassInputs;
		PassInputs.Target = (*Inputs.SceneTextures)->SceneColorTexture;
		PassInputs.TargetType = EKuwaharaFilterTargetType::SceneColor;
		PassInputs.FilterSize = AnimepoyRenderProxy.PrePostProcessKuwaharaFilterSize;

		AddKuwaharaFilterPass(GraphBuilder, View, PassInputs);
	}
}

void FAnimepoySceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass == EPostProcessingPass::Tonemap && ShouldProcessThisView() && AnimepoyRenderProxy.bDiffusionFilter)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateLambda([this](FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& Inputs) ->FScreenPassTexture {
			check(InView.bIsViewInfo);
			auto& View = static_cast<const FViewInfo&>(InView);

			FPostProcessDiffusionInputs PassInputs;
			PassInputs.OverrideOutput = Inputs.OverrideOutput;
			PassInputs.SceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
			PassInputs.PreTonemapColor = (*Inputs.SceneTextures.SceneTextures.GetUniformBuffer())->SceneColorTexture;
			PassInputs.Intensity = AnimepoyRenderProxy.DiffusionFilterIntensity;
			PassInputs.LuminanceMin = AnimepoyRenderProxy.DiffusionLuminanceMin;
			PassInputs.LuminanceMax = AnimepoyRenderProxy.DiffusionLuminanceMax;
			PassInputs.BlurPercentage = AnimepoyRenderProxy.DiffusionBlurPercentage;
			PassInputs.BlendMode = (int32)AnimepoyRenderProxy.DiffusionBlendMode;
			PassInputs.bDebugMask = AnimepoyRenderProxy.bPreviewDiffusionMask;

			return AddPostProcessDiffusionPass(GraphBuilder, View, PassInputs);
			}));
	}
}

bool FAnimepoySceneViewExtension::ShouldProcessThisView() const
{
	return bEnable;
}
