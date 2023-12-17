#pragma once

#include "CoreMinimal.h"
#include "ScreenPass.h"
#include "SceneTexturesConfig.h"

struct FDetectLinePassInputs
{
	FRDGTextureRef GBufferA;
	FRDGTextureRef GBufferB;
	FRDGTextureRef SceneDepth;
	float DepthLineIntensity;
	float NormalLineIntensity;
	float MaterialLineIntensity;
	float PlanarLineIntensity;
	float NonLineSpecular;
};

struct FCompositeLinePassInputs
{
	FRDGTextureRef Target;
	FRDGTextureRef SceneDepth;
	FRDGTextureRef GBufferB;
	FRDGTextureRef LineTexture;
	FLinearColor LineColor;
	int32 LineWidth;
	bool bPreview;
};

FRDGTextureRef AddDetectLinePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FDetectLinePassInputs& Inputs);

void AddCompositeLinePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FCompositeLinePassInputs& Inputs);
