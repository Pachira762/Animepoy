// @Custom
#pragma once

#include "ScreenPass.h"
#include "Animepoy.h"

struct FSketchFilterInputs
{
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
	bool bFilterSceneColor;
	bool bFilterBaseColor;
	bool bFilterWorldNormal;
	ESketchFilterType FilterType;
	int32 FilterSize;
	ESketchFilterDirection FilterDirection;
	bool bDebugFilter;
};

void AddGBufferSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInputs& Inputs);

void AddSceneColorSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInputs& Inputs);
