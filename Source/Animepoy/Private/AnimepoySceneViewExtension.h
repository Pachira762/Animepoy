// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "AnimepoySubsystem.h"

class FAnimepoySceneViewExtension : public FSceneViewExtensionBase
{
public:
	FAnimepoySceneViewExtension(const FAutoRegister& AutoRegister, UAnimepoySubsystem* WorldSubsystem);

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

#if USE_POST_DEFERRED_LIGHTING_PASS
	// Called right after deferred lighting, before fog rendering.
	virtual void PostDeferredLighting_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
#endif // USE_POST_DEFERRED_LIGHTING_PASS

	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	UAnimepoySubsystem* WorldSubsystem{};
	FAnimepoyRenderProxy AnimepoyRenderProxy;
	bool bEnable;

	bool ShouldProcessThisView() const;
};
