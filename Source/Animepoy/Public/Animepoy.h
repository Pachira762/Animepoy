// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Animepoy.generated.h"

UENUM(BlueprintType)
enum class EAnimeDiffusionBlendMode : uint8
{
	Lighten,
	Screen,
	Overlay,
	SoftLight,
};

struct FAnimepoyRenderProxy
{
	bool bEnable;

	// Line Art
	bool bLineArt;
	FLinearColor LineColor;
	int32 LineWidth;
	float DepthLineIntensity;
	float NormalLineIntensity;
	float MaterialLineIntensity;
	float PlanarLineIntensity;
	bool bPreviewLine;

	// Kuwahara Filter
	bool bPrePostProcessKuwaharaFilter;
	int32 PrePostProcessKuwaharaFilterSize;

	// Diffusion Filter
	bool bDiffusionFilter;
	float DiffusionFilterIntensity;
	float DiffusionLuminanceMin;
	float DiffusionLuminanceMax;
	float DiffusionBlurPercentage;
	EAnimeDiffusionBlendMode DiffusionBlendMode;
	bool bPreviewDiffusionMask;
};

UCLASS()
class ANIMEPOY_API AAnimepoy : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art")
	bool bLineArt = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art")
	FLinearColor LineColor = FLinearColor(0.f, 0.f, 0.f, 1.f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art", meta = (ClampMin = "1", ClampMax = "7"))
	int32 LineWidth = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DepthLineIntensity = 0.9f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NormalLineIntensity = 0.75f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlanarLineIntensity = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaterialLineIntensity = 0.75f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art")
	bool bPreviewLine = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter")
	bool bPrePostProcessKuwaharaFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter", meta = (ClampMin = "1", ClampMax = "7"))
	int32 PrePostProcessKuwaharaFilterSize = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter")
	bool bDiffusionFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiffusionFilterIntensity = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiffusionLuminanceMin = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiffusionLuminanceMax = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float DiffusionBlurPercentage = 8.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter")
	EAnimeDiffusionBlendMode DiffusionBlendMode = EAnimeDiffusionBlendMode::Overlay;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter")
	bool bPreviewDiffusionMask = false;

public:
	AAnimepoy();

protected:
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;

	virtual void Tick(float DeltaSeconds);

	virtual bool ShouldTickIfViewportsOnly() const 
	{ 
		return true;  
	}
};
