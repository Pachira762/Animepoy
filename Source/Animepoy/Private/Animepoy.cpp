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
			FAnimepoyRenderProxy Settings;
			Settings.bEnable = !this->IsHidden();

			Settings.bLineArt = bLineArt && LineWidth > 0 && LineColor.A != 0.f;
			Settings.LineColor = LineColor;
			Settings.LineWidth = LineWidth;
			Settings.DepthLineIntensity = DepthLineIntensity;
			Settings.NormalLineIntensity = NormalLineIntensity;
			Settings.MaterialLineIntensity = MaterialLineIntensity;
			Settings.PlanarLineIntensity = PlanarLineIntensity;
			Settings.bPreviewLine = bPreviewLine;

			Settings.bSketchFilter = bSketchFilter && (bFilterSceneColor || bFilterBaseColor || bFilterWorldNormal) && SketchFilterSize > 0;
			Settings.bFilterSceneColor = bFilterSceneColor;
			Settings.bFilterBaseColor = bFilterBaseColor;
			Settings.bFilterWorldNormal = bFilterWorldNormal;
			Settings.SketchFilterType = SketchFilterType;
			Settings.SketchFilterSize = SketchFilterSize;
			Settings.SketchFilterDirection = SketchFilterDirection;
			Settings.bDebugSketchFilter = bDebugSketchFilter;

			Settings.bDiffusionFilter = bDiffusionFilter && DiffusionFilterIntensity != 0.f;
			Settings.DiffusionFilterIntensity = DiffusionFilterIntensity;
			Settings.DiffusionLuminanceMin = DiffusionLuminanceMin;
			Settings.DiffusionLuminanceMax = DiffusionLuminanceMax;
			Settings.DiffusionBlurPercentage = DiffusionBlurPercentage;
			Settings.DiffusionBlendMode = DiffusionBlendMode;
			Settings.bPreviewDiffusionMask = bPreviewDiffusionMask;

			AnimepoySubsystem->SetAnimepoyRenderProxy(Settings);
		}
	}
}
