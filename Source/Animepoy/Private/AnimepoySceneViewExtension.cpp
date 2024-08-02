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
#include "LineArt.h"
#include "SketchFilter.h"
#include "DiffusionFilter.h"

namespace
{
	FRDGTextureRef GetOrientationTexture(ESketchFilterOrientation Orientation, FRDGTextureRef ColorTexture, FRDGTextureRef DepthTexture)
	{
		switch (Orientation)
		{
		case ESketchFilterOrientation::Color: return ColorTexture;
		case ESketchFilterOrientation::Depth: return DepthTexture;
		default: return nullptr;
		}
	}
}

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

void FAnimepoySceneViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	check(InView.bIsViewInfo);
	auto& View = static_cast<const FViewInfo&>(InView);

	if (ShouldProcessThisView())
	{
		if (AnimepoyRenderProxy.bSketchFilter && AnimepoyRenderProxy.BaseColorSketchFilterSettings.bEnabled)
		{
			const FSketchFilterSettings& Settings = AnimepoyRenderProxy.BaseColorSketchFilterSettings;

			FSketchFilterInput PassInputs;
			PassInputs.RenderTarget = RenderTargets[3]; // GBufferC
			PassInputs.TargetType = ESketchFilterTargetType::Color;
			PassInputs.FilterMethod = Settings.FilterMethod;
			PassInputs.FilterSize = Settings.FilterSize;
			PassInputs.Orientation = Settings.Orientation;
			PassInputs.OrientationTexture = GetOrientationTexture(Settings.Orientation, RenderTargets[3].GetTexture(), RenderTargets.DepthStencil.GetTexture());

			AddSketchFilterPass(GraphBuilder, View, PassInputs);
		}

		if (AnimepoyRenderProxy.bSketchFilter && AnimepoyRenderProxy.WorldNormalSketchFilterSettings.bEnabled)
		{
			const FSketchFilterSettings& Settings = AnimepoyRenderProxy.WorldNormalSketchFilterSettings;

			FSketchFilterInput PassInputs;
			PassInputs.RenderTarget = RenderTargets[1]; // GBufferA
			PassInputs.TargetType = ESketchFilterTargetType::Normal;
			PassInputs.FilterMethod = Settings.FilterMethod;
			PassInputs.FilterSize = Settings.FilterSize;
			PassInputs.Orientation = Settings.Orientation;
			PassInputs.OrientationTexture = GetOrientationTexture(Settings.Orientation, RenderTargets[3].GetTexture(), RenderTargets.DepthStencil.GetTexture());

			AddSketchFilterPass(GraphBuilder, View, PassInputs);
		}
	}
}

#if USE_POST_DEFERRED_LIGHTING_PASS
void FAnimepoySceneViewExtension::PostDeferredLighting_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	check(InView.bIsViewInfo);
	auto& View = static_cast<const FViewInfo&>(InView);

	if(ShouldProcessThisView())
	{
		if (AnimepoyRenderProxy.bSketchFilter && AnimepoyRenderProxy.SceneColorSketchFilterSettings.bEnabled)
		{
			const FSketchFilterSettings& Settings = AnimepoyRenderProxy.SceneColorSketchFilterSettings;

			FSketchFilterInput PassInputs;
			PassInputs.RenderTarget = FRenderTargetBinding((*SceneTextures)->SceneColorTexture, ERenderTargetLoadAction::ELoad);
			PassInputs.TargetType = ESketchFilterTargetType::Color;
			PassInputs.FilterMethod = Settings.FilterMethod;
			PassInputs.FilterSize = Settings.FilterSize;
			PassInputs.Orientation = Settings.Orientation;
			PassInputs.OrientationTexture = GetOrientationTexture(Settings.Orientation, (*SceneTextures)->SceneColorTexture, (*SceneTextures)->SceneDepthTexture);

			AddSketchFilterPass(GraphBuilder, View, PassInputs);
		}

		if (AnimepoyRenderProxy.bLineArt)
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
}
#endif // USE_POST_DEFERRED_LIGHTING_PASS

void FAnimepoySceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs)
{
#if !USE_POST_DEFERRED_LIGHTING_PASS
	check(InView.bIsViewInfo);
	auto& View = static_cast<const FViewInfo&>(InView);

	if (ShouldProcessThisView() && AnimepoyRenderProxy.bSketchFilter)
	{
		FPostProcessSketchFilterInputs PassInputs;
		PassInputs.SceneTextures = SceneTextures;
		PassInputs.FilterType = AnimepoyRenderProxy.SketchFilterType;
		PassInputs.FilterSize = AnimepoyRenderProxy.SketchFilterSize;
		PassInputs.FilterDirection = AnimepoyRenderProxy.SketchFilterDirection;
		PassInputs.bDebugFilter = AnimepoyRenderProxy.bDebugSketchFilter;

		AddSketchFilterPass(GraphBuilder, View, PassInputs);
	}

	if (ShouldProcessThisView() && AnimepoyRenderProxy.bLineArt)
	{
		FLineArtPassInputs PassInputs;
		PassInputs.SceneTextures = Inputs.SceneTextures;
		PassInputs.DepthLineIntensity = AnimepoyRenderProxy.DepthLineIntensity;
		PassInputs.NormalLineIntensity = AnimepoyRenderProxy.NormalLineIntensity;
		PassInputs.PlanarLineIntensity = AnimepoyRenderProxy.PlanarLineIntensity;
		PassInputs.MaterialLineIntensity = AnimepoyRenderProxy.MaterialLineIntensity;
		PassInputs.LineWidth = AnimepoyRenderProxy.LineWidth;
		PassInputs.LineColor = AnimepoyRenderProxy.LineColor;
		PassInputs.bPreview = AnimepoyRenderProxy.bPreviewLine;

		AddLineArtPass(GraphBuilder, View, PassInputs);
	}
#endif // !USE_POST_DEFERRED_LIGHTING_PASS
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
			PassInputs.BlendMode = AnimepoyRenderProxy.DiffusionBlendMode;
			PassInputs.bDebugMask = AnimepoyRenderProxy.bPreviewDiffusionMask;

			return AddPostProcessDiffusionPass(GraphBuilder, View, PassInputs);
			}));
	}
}

bool FAnimepoySceneViewExtension::ShouldProcessThisView() const
{
	return bEnable;
}
