// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnimepoySettings.generated.h"

UENUM(BlueprintType)
enum class EAnimeDiffusionBlendMode
{
	Lighten,
	Screen,
	Overlay,
	SoftLight,
};

UCLASS()
class ANIMEPOY_API AAnimepoySettings : public AActor
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
	float MaterialLineIntensity = 0.75f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlanarLineIntensity = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art")
	float NonLineSpecular = -1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art")
	bool bDrawToBaseColor = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line Art")
	bool bPreviewLine = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter")
	bool bKuwaharaFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter")
	bool bSceneColorKuwaharaFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter", meta = (ClampMin = "1", ClampMax = "7"))
	int32 SceneColorKuwaharaFilterSize = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter")
	bool bBaseColorKuwaharaFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter", meta = (ClampMin = "1", ClampMax = "7"))
	int32 BaseColorKuwaharaFilterSize = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter")
	bool bWorldNormalKuwaharaFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter", meta = (ClampMin = "1", ClampMax = "7"))
	int32 WorldNormalKuwaharaFilterSize = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter")
	bool bMaterialKuwaharaFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Kuwahara Filter", meta = (ClampMin = "1", ClampMax = "7"))
	int32 MaterialKuwaharaFilterSize = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter")
	bool bDiffusionFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiffusionFilterIntensity = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiffusionLuminanceMin = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiffusionLuminanceMax = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter")
	bool bPreTonemapLuminance = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float DiffusionBlurPercentage = 8.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter")
	EAnimeDiffusionBlendMode DiffusionBlendMode = EAnimeDiffusionBlendMode::Overlay;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Diffusion Filter")
	bool bDebugDiffusionMask = false;

public:
	AAnimepoySettings();

protected:
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;
};
