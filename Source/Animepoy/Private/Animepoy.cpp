// Fill out your copyright notice in the Description page of Project Settings.


#include "Animepoy.h"
#include "AnimepoySubsystem.h"

// Sets default values
AAnimepoy::AAnimepoy()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AAnimepoy::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		if (UAnimepoySubsystem* WorldSubsystem = World->GetSubsystem<UAnimepoySubsystem>())
		{
			WorldSubsystem->OnActorSpawned(this);
		}
	}
}

void AAnimepoy::BeginDestroy()
{
	if (const UWorld* World = GetWorld())
	{
		if (UAnimepoySubsystem* WorldSubsystem = World->GetSubsystem<UAnimepoySubsystem>())
		{
			WorldSubsystem->OnActorDeleted(this);
		}
	}

	Super::BeginDestroy();
}

void AAnimepoy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (const UWorld* World = GetWorld()) 
	{
		if (UAnimepoySubsystem* AnimepoySubsystem = World->GetSubsystem<UAnimepoySubsystem>()) 
		{
			FAnimepoyRenderProxy TempSettings;
			TempSettings.bEnable = !this->IsHidden();

			TempSettings.bLineArt = bLineArt && LineWidth > 0 && LineColor.A != 0.f;
			TempSettings.LineColor = LineColor;
			TempSettings.LineWidth = LineWidth;
			TempSettings.DepthLineIntensity = DepthLineIntensity;
			TempSettings.NormalLineIntensity = NormalLineIntensity;
			TempSettings.MaterialLineIntensity = MaterialLineIntensity;
			TempSettings.PlanarLineIntensity = PlanarLineIntensity;
			TempSettings.bPreviewLine = bPreviewLine;

			TempSettings.bPrePostProcessKuwaharaFilter = bPrePostProcessKuwaharaFilter;
			TempSettings.PrePostProcessKuwaharaFilterSize = PrePostProcessKuwaharaFilterSize;

			TempSettings.bDiffusionFilter = bDiffusionFilter && DiffusionFilterIntensity != 0.f;
			TempSettings.DiffusionFilterIntensity = DiffusionFilterIntensity;
			TempSettings.DiffusionLuminanceMin = DiffusionLuminanceMin;
			TempSettings.DiffusionLuminanceMax = DiffusionLuminanceMax;
			TempSettings.DiffusionBlurPercentage = DiffusionBlurPercentage;
			TempSettings.DiffusionBlendMode = DiffusionBlendMode;
			TempSettings.bPreviewDiffusionMask = bPreviewDiffusionMask;

			AnimepoySubsystem->SetAnimepoyRenderProxy(TempSettings);
		}
	}
}
