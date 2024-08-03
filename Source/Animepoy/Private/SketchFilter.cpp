// @Custom
#include "SketchFilter.h"
#include "PostProcess/PostProcessDownsample.h"
#include "PostProcess/PostProcessWeightedSampleSum.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"
#include "SceneRendering.h"
#include "SceneTextureParameters.h"
#include "PixelShaderUtils.h"
#include "RenderGraphUtils.h"
#include "UnrealEngine.h"

namespace {
	class FTargetType : SHADER_PERMUTATION_ENUM_CLASS("TARGET_TYPE", ESketchFilterTargetType);
	class FFilterMethod : SHADER_PERMUTATION_ENUM_CLASS("FILTER_METHOD", ESketchFilterMethod);
	class FOrientation : SHADER_PERMUTATION_ENUM_CLASS("ORIENTATION", ESketchFilterOrientation);

	class FSketchFilterPS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FSketchFilterPS);
		SHADER_USE_PARAMETER_STRUCT(FSketchFilterPS, FGlobalShader);

		using FPermutationDomain = TShaderPermutationDomain<FTargetType, FFilterMethod, FOrientation>;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER(int32, FilterSize)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, OrientationTexture)
			RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()
	};

	IMPLEMENT_GLOBAL_SHADER(FSketchFilterPS, "/AnimepoyShaders/Private/SketchFilter.usf", "SketchFilterPS", SF_Pixel);
}

void AddSketchFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSketchFilterInput& Inputs)
{
	if (Inputs.FilterMethod == ESketchFilterMethod::AnisotropicKuwahara && !Inputs.OrientationTexture)
	{
		return;
	}
	
	RDG_EVENT_SCOPE(GraphBuilder, "SketchFilter");

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport(View.ViewRect);

	FRDGTextureRef TargetTexture = Inputs.RenderTarget.GetTexture();
	FRDGTextureRef InputTexture = GraphBuilder.CreateTexture(TargetTexture->Desc, TEXT("SketchFilterInput"));
	AddCopyTexturePass(GraphBuilder, TargetTexture, InputTexture);

	FRDGTextureRef OrientationTexture = Inputs.OrientationTexture;
	if (OrientationTexture == TargetTexture)
	{
		OrientationTexture = InputTexture;
	}

	FSketchFilterPS::FPermutationDomain PermutationVector;
	PermutationVector.Set<FTargetType>(Inputs.TargetType);
	PermutationVector.Set<FFilterMethod>(Inputs.FilterMethod);
	PermutationVector.Set<FOrientation>(Inputs.Orientation);

	FSketchFilterPS::FParameters* Parameters = GraphBuilder.AllocParameters<FSketchFilterPS::FParameters>();
	Parameters->View = View.ViewUniformBuffer;
	Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
	Parameters->FilterSize = Inputs.FilterSize;
	Parameters->InputTexture = InputTexture;
	Parameters->OrientationTexture = OrientationTexture;
	Parameters->RenderTargets[0] = Inputs.RenderTarget;

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		ShaderMap,
		RDG_EVENT_NAME("SketchFilterPS"),
		TShaderMapRef<FSketchFilterPS>(ShaderMap, PermutationVector),
		Parameters,
		Viewport.Rect,
		TStaticBlendState<CW_RGB>::GetRHI());
}
