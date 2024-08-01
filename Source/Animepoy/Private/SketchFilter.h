// @Custom
#pragma once

#include "ScreenPass.h"
#include "Animepoy.h"

struct FGBufferSketchFilterInputs
{
	const FRenderTargetBindingSlots& RenderTargets;
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
	bool bProcessBaseColor;
	bool bProcessWorldNormal;
	ESketchFilterType FilterType;
	int32 FilterSize;
	ESketchFilterDirection FilterDirection;
	bool bDebugFilter;
};

struct FPostProcessSketchFilterInputs
{
	TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
	ESketchFilterType FilterType;
	int32 FilterSize;
	ESketchFilterDirection FilterDirection;
	bool bDebugFilter;
};

void AddGBufferSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FGBufferSketchFilterInputs& Inputs);

void AddSceneColorSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FPostProcessSketchFilterInputs& Inputs);
