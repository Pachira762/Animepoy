// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AnimepoySettings.h"
#include "AnimepoyWorldSubsystem.generated.h"

struct FAnimepoySettingsRenderProxy
{
	// Line Art
	bool bLineArtEnabled;
	FLinearColor LineColor;
	int32 LineWidth;
	float DepthLineIntensity;
	float NormalLineIntensity;
	float MaterialLineIntensity;
	float PlanarLineIntensity;
	float NonLineSpecular;
	bool bDrawToBaseColor;
	bool bPreviewLine;

	// Kuwahara Filter
	bool bSceneColorKuwaharaFilter;
	int32 SceneColorKuwaharaFilterSize;
	bool bBaseColorKuwaharaFilter;
	int32 BaseColorKuwaharaFilterSize;
	bool bWorldNormalKuwaharaFilter;
	int32 WorldNormalKuwaharaFilterSize;
	bool bMaterialKuwaharaFilter;
	int32 MaterialKuwaharaFilterSize;

	// Diffusion Filter
	bool bDiffusionFilterEnabled;
	float DiffusionFilterIntensity;
	float DiffusionLuminanceMin;
	float DiffusionLuminanceMax;
	bool bPreTonemapLuminance;
	float DiffusionBlurPercentage;
	EAnimeDiffusionBlendMode DiffusionBlendMode;
	bool bDebugDiffusionMask;
};

/**
 *
 */
UCLASS()
class ANIMEPOY_API UAnimepoyWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection)override;

	virtual void Deinitialize()override;

	virtual bool IsTickableInEditor() const override
	{
		return true;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UAnimepoyWorldSubsystem, STATGROUP_Tickables);
	}

	virtual void Tick(float DeltaTime) override;

public:
	void OnActorSpawned(AActor* Actor);

	void OnActorDeleted(AActor* Actor);

	void OnActorListChanged();

	FAnimepoySettingsRenderProxy GetSettings() const
	{
		FScopeLock Lock(&Mutex);
		return Settings;
	}

	bool IsEffectsEnabled() const
	{
		FScopeLock Lock(&Mutex);
		return Settings.bLineArtEnabled || Settings.bDiffusionFilterEnabled || Settings.bSceneColorKuwaharaFilter || Settings.bBaseColorKuwaharaFilter || Settings.bWorldNormalKuwaharaFilter || Settings.bMaterialKuwaharaFilter;
	}

private:
	TSharedPtr<class FAnimepoySceneViewExtension, ESPMode::ThreadSafe> AnimepoySceneViewExtension;

	AAnimepoySettings* SettingsActor;

	mutable FCriticalSection Mutex;

	FAnimepoySettingsRenderProxy Settings = {};
};
