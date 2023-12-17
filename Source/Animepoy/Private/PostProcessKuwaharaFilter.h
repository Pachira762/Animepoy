// @Custom
#pragma once

#include "ScreenPass.h"

enum class EKuwaharaFilterTargetType
{
	SceneColor,
	BaseColor,
	Normal,
	Material,
};

struct FKuwaharaFilterInputs
{
	FRDGTextureRef Target;
	EKuwaharaFilterTargetType TargetType;
	int32 FilterSize;
};

void AddKuwaharaFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FKuwaharaFilterInputs& Inputs);
