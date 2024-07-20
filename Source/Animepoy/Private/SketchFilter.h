// @Custom
#pragma once

#include "ScreenPass.h"
#include "Animepoy.h"

struct FSketchFilterInputs
{
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
	int32 FilterSize;
	ESketchFilterType FilterType;
	ESketchFilterTarget FilterTarget;
	bool bDebugFilter;
};

void AddSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInputs& Inputs);
