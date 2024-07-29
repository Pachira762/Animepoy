// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Animepoy.generated.h"

UENUM(BlueprintType)
enum class ESketchFilterType : uint8
{
	Kuwahara,
	SymmetricNearestNeighbor,
};

UENUM(BlueprintType)
enum class ESketchFilterDirection : uint8
{
	None,
	Color,
	CrossColor,
	Normal,
	CrossNormal,
	Depth,
	DepthNormal,
};

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

	// Sketch Filter
	bool bSketchFilter;
	bool bFilterSceneColor;
	bool bFilterBaseColor;
	bool bFilterWorldNormal;
	ESketchFilterType SketchFilterType;
	int32 SketchFilterSize;
	ESketchFilterDirection SketchFilterDirection;
	bool bDebugSketchFilter;

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

	//
	// Line Art
	//

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

	//
	// Sketch Filter
	//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sketch Filter")
	bool bSketchFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sketch Filter")
	bool bFilterSceneColor = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sketch Filter")
	bool bFilterBaseColor = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sketch Filter")
	bool bFilterWorldNormal = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sketch Filter")
	ESketchFilterType SketchFilterType = ESketchFilterType::Kuwahara;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sketch Filter", meta = (ClampMin = "1", ClampMax = "7"))
	int32 SketchFilterSize = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sketch Filter")
	ESketchFilterDirection SketchFilterDirection = ESketchFilterDirection::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sketch Filter")
	bool bDebugSketchFilter = false;

	//
	// Diffusion Filter
	//

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
