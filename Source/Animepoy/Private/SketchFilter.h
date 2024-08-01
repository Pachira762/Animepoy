// @Custom
#pragma once

#include "ScreenPass.h"
#include "Animepoy.h"

enum class ESketchFilterTargetType : uint8
{
	Color,
	Normal,
	MAX,
};

struct FSketchFilterInput
{
	FRenderTargetBinding RenderTarget;
	ESketchFilterTargetType TargetType;
	ESketchFilterMethod FilterMethod;
	int32 FilterSize;
	ESketchFilterOrientation Orientation;
	FRDGTextureRef OrientationTexture;
};

void AddSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInput& Inputs);