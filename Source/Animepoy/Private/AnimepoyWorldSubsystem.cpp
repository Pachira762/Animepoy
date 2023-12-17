// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimepoyWorldSubsystem.h"
#include "EngineUtils.h"
#include "AnimepoySettings.h"
#include "AnimepoySceneViewExtension.h"

void UAnimepoyWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	AnimepoySceneViewExtension = FSceneViewExtensions::NewExtension<FAnimepoySceneViewExtension>(this);

#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		GEngine->OnLevelActorAdded().AddUObject(this, &UAnimepoyWorldSubsystem::OnActorSpawned);
		GEngine->OnLevelActorDeleted().AddUObject(this, &UAnimepoyWorldSubsystem::OnActorDeleted);
		GEngine->OnLevelActorListChanged().AddUObject(this, &UAnimepoyWorldSubsystem::OnActorListChanged);
	}
#endif
}

void UAnimepoyWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();

#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		GEngine->OnLevelActorAdded().RemoveAll(this);
		GEngine->OnLevelActorDeleted().RemoveAll(this);
		GEngine->OnLevelActorListChanged().RemoveAll(this);
	}
#endif
}

void UAnimepoyWorldSubsystem::Tick(float DeltaTime)
{
	if (SettingsActor)
	{
		FAnimepoySettingsRenderProxy TempSettings;
		TempSettings.bLineArtEnabled = SettingsActor->bLineArt && SettingsActor->LineWidth > 0 && SettingsActor->LineColor.A != 0.f;
		TempSettings.LineColor = SettingsActor->LineColor;
		TempSettings.LineWidth = SettingsActor->LineWidth;
		TempSettings.DepthLineIntensity = SettingsActor->DepthLineIntensity;
		TempSettings.NormalLineIntensity = SettingsActor->NormalLineIntensity;
		TempSettings.MaterialLineIntensity = SettingsActor->MaterialLineIntensity;
		TempSettings.PlanarLineIntensity = SettingsActor->PlanarLineIntensity;
		TempSettings.NonLineSpecular = SettingsActor->NonLineSpecular;
		TempSettings.bDrawToBaseColor = SettingsActor->bDrawToBaseColor;
		TempSettings.bPreviewLine = SettingsActor->bPreviewLine;

		bool bSceneColorKuwaharaFilter = SettingsActor->bKuwaharaFilter && SettingsActor->bSceneColorKuwaharaFilter;
		bool bBaseColorKuwaharaFitler = SettingsActor->bKuwaharaFilter && SettingsActor->bBaseColorKuwaharaFilter;
		bool bWorldNormalKuwaharaFilter = SettingsActor->bKuwaharaFilter && SettingsActor->bWorldNormalKuwaharaFilter;
		bool bMetallicRoughnessKuwaharaFilter = SettingsActor->bKuwaharaFilter && SettingsActor->bMaterialKuwaharaFilter;
		TempSettings.bSceneColorKuwaharaFilter = bSceneColorKuwaharaFilter;
		TempSettings.SceneColorKuwaharaFilterSize = SettingsActor->SceneColorKuwaharaFilterSize;
		TempSettings.bBaseColorKuwaharaFilter = bBaseColorKuwaharaFitler;
		TempSettings.BaseColorKuwaharaFilterSize = SettingsActor->BaseColorKuwaharaFilterSize;
		TempSettings.bWorldNormalKuwaharaFilter = bWorldNormalKuwaharaFilter;
		TempSettings.WorldNormalKuwaharaFilterSize = SettingsActor->WorldNormalKuwaharaFilterSize;
		TempSettings.bMaterialKuwaharaFilter = bMetallicRoughnessKuwaharaFilter;
		TempSettings.MaterialKuwaharaFilterSize = SettingsActor->MaterialKuwaharaFilterSize;

		TempSettings.bDiffusionFilterEnabled = SettingsActor->bDiffusionFilter && SettingsActor->DiffusionFilterIntensity != 0.f;
		TempSettings.DiffusionFilterIntensity = SettingsActor->DiffusionFilterIntensity;
		TempSettings.DiffusionLuminanceMin = SettingsActor->DiffusionLuminanceMin;
		TempSettings.DiffusionLuminanceMax = SettingsActor->DiffusionLuminanceMax;
		TempSettings.bPreTonemapLuminance = SettingsActor->bPreTonemapLuminance;
		TempSettings.DiffusionBlurPercentage = SettingsActor->DiffusionBlurPercentage;
		TempSettings.DiffusionBlendMode = SettingsActor->DiffusionBlendMode;
		TempSettings.bDebugDiffusionMask = SettingsActor->bDebugDiffusionMask;

		FScopeLock Lock(&Mutex);
		Settings = TempSettings;
	}
}

void UAnimepoyWorldSubsystem::OnActorSpawned(AActor* Actor)
{
	AAnimepoySettings* AsSettingsActor = Cast<AAnimepoySettings>(Actor);
	if (AsSettingsActor)
	{
		SettingsActor = AsSettingsActor;
	}
}

void UAnimepoyWorldSubsystem::OnActorDeleted(AActor* Actor)
{
	if ((AActor*)SettingsActor == Actor)
	{
		SettingsActor = nullptr;
	}
}

void UAnimepoyWorldSubsystem::OnActorListChanged()
{
	for (TActorIterator<AAnimepoySettings> It(GetWorld()); It; ++It)
	{
		SettingsActor = *It;
		break;
	}
}
