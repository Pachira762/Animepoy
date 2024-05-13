// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "AnimepoySubsystem.h"

#define CUSTOM_SCENE_VIEW_EXTENSION 1

class FAnimepoySceneViewExtension : public FSceneViewExtensionBase
{
public:
	FAnimepoySceneViewExtension(const FAutoRegister& AutoRegister, UAnimepoySubsystem* WorldSubsystem);

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily);

	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) {}

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) {}

#if CUSTOM_SCENE_VIEW_EXTENSION
	// Called right after deferred lighting, before fog rendering.
	virtual void PostDeferredLighting_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
#endif

	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	UAnimepoySubsystem* WorldSubsystem{};

	FAnimepoyRenderProxy AnimepoyRenderProxy;

	FScreenPassTexture DiffusionFilterPass(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& Inputs);

	bool ShouldProcessThisView();

protected:
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const 
	{
		return Context.GetWorld() == WorldSubsystem->GetWorld();
	}

};
