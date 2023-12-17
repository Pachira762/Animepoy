// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "AnimepoyWorldSubsystem.h"

class FAnimepoySceneViewExtension : public FSceneViewExtensionBase
{
public:
	FAnimepoySceneViewExtension(const FAutoRegister& AutoRegister, UAnimepoyWorldSubsystem* WorldSubsystem);

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily);

	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) {}

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) {}

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;

	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

private:
	UAnimepoyWorldSubsystem* WorldSubsystem{};

	FAnimepoySettingsRenderProxy Settings{};

	FRDGTextureRef LineTexture{};

	FScreenPassTexture DiffusionFilterPass(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& Inputs);

	bool ShouldProcessThisView(const FSceneView& View);
};
