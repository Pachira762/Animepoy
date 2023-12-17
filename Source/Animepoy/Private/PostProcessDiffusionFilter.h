// @Custom

#pragma once

#include "ScreenPass.h"

class FSceneTextureParameters;

struct FPostProcessDiffusionInputs
{
	FScreenPassRenderTarget OverrideOutput;
	FScreenPassTexture SceneColor;
	FRDGTextureRef PreTonemapColor;
	float Intensity;
	float LuminanceMin;
	float LuminanceMax;
	bool bPreTonemapLuminance;
	float BlurPercentage;
	int32 BlendMode;
	bool bDebugMask;
};

FScreenPassTexture AddPostProcessDiffusionPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, FPostProcessDiffusionInputs& Inputs);