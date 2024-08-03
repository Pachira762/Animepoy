// @Custom

#pragma once

#include "ScreenPass.h"
#include "Animepoy.h"

class FSceneTextureParameters;

struct FDiffusionFilterInputs
{
	FScreenPassRenderTarget OverrideOutput;
	FScreenPassTexture SceneColor;
	float Intensity;
	float LuminanceMin;
	float LuminanceMax;
	float BlurPercentage;
	EDiffusionFilterBlendMode BlendMode;
	bool bPreviewMask;
};

FScreenPassTexture AddDiffusionFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, FDiffusionFilterInputs& Inputs);