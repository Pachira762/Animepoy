// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimepoySceneViewExtension.h"
#include "ShaderCompilerCore.h"
#include "ScreenPass.h"
#include "SceneTexturesConfig.h"
#include "SceneTextureParameters.h"
#include "ShaderParameterUtils.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "AnimepoyWorldSubsystem.h"
#include "AnimepoySettings.h"
#include "PostProcessLineArt.h"
#include "PostProcessKuwaharaFilter.h"
#include "PostProcessDiffusionFilter.h"

FAnimepoySceneViewExtension::FAnimepoySceneViewExtension(const FAutoRegister& AutoRegister, UAnimepoyWorldSubsystem* WorldSubsystem)
	: FSceneViewExtensionBase(AutoRegister)
	, WorldSubsystem(WorldSubsystem)
{
}

void FAnimepoySceneViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	Settings = WorldSubsystem->GetSettings();
}

void FAnimepoySceneViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	check(InView.bIsViewInfo);

	auto& View = static_cast<const FViewInfo&>(InView);

	if (!ShouldProcessThisView(View))
	{
		return;
	}

	if (Settings.bLineArtEnabled)
	{
		FDetectLinePassInputs PassInputs;
		PassInputs.GBufferA = RenderTargets[1].GetTexture();
		PassInputs.GBufferB = RenderTargets[2].GetTexture();
		PassInputs.SceneDepth = RenderTargets.DepthStencil.GetTexture();
		PassInputs.DepthLineIntensity = FMath::Clamp(Settings.DepthLineIntensity, 0.f, 1.f);
		PassInputs.NormalLineIntensity = FMath::Clamp(Settings.NormalLineIntensity, 0.f, 1.f);
		PassInputs.MaterialLineIntensity = FMath::Clamp(Settings.MaterialLineIntensity, 0.f, 1.f);
		PassInputs.PlanarLineIntensity = FMath::Clamp(Settings.PlanarLineIntensity, 0.f, 1.f);
		PassInputs.NonLineSpecular = Settings.NonLineSpecular;

		LineTexture = AddDetectLinePass(GraphBuilder, View, PassInputs);
	}

	if (Settings.bBaseColorKuwaharaFilter)
	{
		FKuwaharaFilterInputs PassInputs;
		PassInputs.Target = RenderTargets[3].GetTexture();
		PassInputs.TargetType = EKuwaharaFilterTargetType::BaseColor;
		PassInputs.FilterSize = Settings.BaseColorKuwaharaFilterSize;

		AddKuwaharaFilterPass(GraphBuilder, View, PassInputs);
	}

	if (Settings.bWorldNormalKuwaharaFilter)
	{
		FKuwaharaFilterInputs PassInputs;
		PassInputs.Target = RenderTargets[1].GetTexture();
		PassInputs.TargetType = EKuwaharaFilterTargetType::Normal;
		PassInputs.FilterSize = Settings.WorldNormalKuwaharaFilterSize;

		AddKuwaharaFilterPass(GraphBuilder, View, PassInputs);
	}

	if (Settings.bMaterialKuwaharaFilter)
	{
		FKuwaharaFilterInputs PassInputs;
		PassInputs.Target = RenderTargets[2].GetTexture(); // GBufferB
		PassInputs.TargetType = EKuwaharaFilterTargetType::Material;
		PassInputs.FilterSize = Settings.MaterialKuwaharaFilterSize;

		AddKuwaharaFilterPass(GraphBuilder, View, PassInputs);
	}

	if (LineTexture && (Settings.bDrawToBaseColor || Settings.bPreviewLine))
	{
		FCompositeLinePassInputs PassInputs;
		PassInputs.Target = Settings.bPreviewLine ? RenderTargets[0].GetTexture() : RenderTargets[3].GetTexture(); // SceneColor or BaseColorTexture
		PassInputs.GBufferB = RenderTargets[2].GetTexture();
		PassInputs.SceneDepth = RenderTargets.DepthStencil.GetTexture();
		PassInputs.LineTexture = LineTexture;
		PassInputs.LineColor = Settings.LineColor;
		PassInputs.LineWidth = Settings.LineWidth;
		PassInputs.bPreview = Settings.bPreviewLine;

		AddCompositeLinePass(GraphBuilder, View, PassInputs);

		LineTexture = nullptr;
	}
}

void FAnimepoySceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs)
{
	check(InView.bIsViewInfo);

	auto& View = static_cast<const FViewInfo&>(InView);

	if (!ShouldProcessThisView(View))
	{
		return;
	}

	if (Settings.bSceneColorKuwaharaFilter)
	{
		FKuwaharaFilterInputs PassInputs;
		PassInputs.Target = (*Inputs.SceneTextures)->SceneColorTexture;
		PassInputs.TargetType = EKuwaharaFilterTargetType::SceneColor;
		PassInputs.FilterSize = Settings.SceneColorKuwaharaFilterSize;

		AddKuwaharaFilterPass(GraphBuilder, View, PassInputs);
	}

	if (LineTexture)
	{
		FCompositeLinePassInputs PassInputs;
		PassInputs.Target = (*Inputs.SceneTextures)->SceneColorTexture;
		PassInputs.GBufferB = nullptr;
		PassInputs.SceneDepth = (*Inputs.SceneTextures)->SceneDepthTexture;
		PassInputs.LineTexture = LineTexture;
		PassInputs.LineColor = Settings.LineColor;
		PassInputs.LineWidth = Settings.LineWidth;
		PassInputs.bPreview = Settings.bPreviewLine;

		AddCompositeLinePass(GraphBuilder, View, PassInputs);

		LineTexture = nullptr;
	}
}

void FAnimepoySceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass == EPostProcessingPass::Tonemap && Settings.bDiffusionFilterEnabled)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FAnimepoySceneViewExtension::DiffusionFilterPass));
	}
}

bool FAnimepoySceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return Context.GetWorld() == WorldSubsystem->GetWorld() && WorldSubsystem->IsEffectsEnabled();
}

FScreenPassTexture FAnimepoySceneViewExtension::DiffusionFilterPass(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& Inputs)
{
	check(InView.bIsViewInfo);
	auto& View = static_cast<const FViewInfo&>(InView);

	FPostProcessDiffusionInputs PassInputs;
	PassInputs.OverrideOutput = Inputs.OverrideOutput;
	PassInputs.SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
	PassInputs.PreTonemapColor = (*Inputs.SceneTextures.SceneTextures.GetUniformBuffer())->SceneColorTexture;
	PassInputs.Intensity = Settings.DiffusionFilterIntensity;
	PassInputs.LuminanceMin = Settings.DiffusionLuminanceMin;
	PassInputs.LuminanceMax = Settings.DiffusionLuminanceMax;
	PassInputs.bPreTonemapLuminance = Settings.bPreTonemapLuminance;
	PassInputs.BlurPercentage = Settings.DiffusionBlurPercentage;
	PassInputs.BlendMode = (int32)Settings.DiffusionBlendMode;
	PassInputs.bDebugMask = Settings.bDebugDiffusionMask;

	return AddPostProcessDiffusionPass(GraphBuilder, View, PassInputs);
}

bool FAnimepoySceneViewExtension::ShouldProcessThisView(const FSceneView& View)
{
	return View.Family->Scene->GetWorld() == WorldSubsystem->GetWorld() && View.Family->EngineShowFlags.PostProcessing && View.Family->EngineShowFlags.PostProcessMaterial;
}
