#pragma once

#include "CoreMinimal.h"
#include "ScreenPass.h"
#include "SceneTexturesConfig.h"

struct FLineArtPassInputs
{
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
	float DepthLineIntensity;
	float NormalLineIntensity;
	float MaterialLineIntensity;
	float PlanarLineIntensity;
	float NonLineSpecular;
	int32 LineWidth;
	FLinearColor LineColor;
	bool bPreview;
};

void AddLineArtPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FLineArtPassInputs& Inputs);
