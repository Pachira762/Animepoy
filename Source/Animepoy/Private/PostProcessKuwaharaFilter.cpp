// @Custom
#include "PostProcessKuwaharaFilter.h"
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
	enum EValueType
	{
		Color,
		Normal,
		Material,
		MAX
	};

	class FValueType : SHADER_PERMUTATION_ENUM_CLASS("VALUE_TYPE", EValueType);
	using FCommonDomain = TShaderPermutationDomain<FValueType>;

	class FKuwaharaFilterSetupCS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FKuwaharaFilterSetupCS);
		SHADER_USE_PARAMETER_STRUCT(FKuwaharaFilterSetupCS, FGlobalShader);

		using FPermutationDomain = FCommonDomain;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
			SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutSummedAreaTableTexture)
			END_SHADER_PARAMETER_STRUCT()

			static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
		}
	};

	IMPLEMENT_GLOBAL_SHADER(FKuwaharaFilterSetupCS, "/AnimepoyShaders/Private/PostProcessKuwaharaFilter.usf", "KuwaharaFilterSetupCS", SF_Compute);

	class FKuwaharaFilterCS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FKuwaharaFilterCS);
		SHADER_USE_PARAMETER_STRUCT(FKuwaharaFilterCS, FGlobalShader);

		using FPermutationDomain = FCommonDomain;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SummedAreaTableTexture)
			SHADER_PARAMETER(int32, FilterSize)
			SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutTexture)
			END_SHADER_PARAMETER_STRUCT()

			static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& Environment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Parameters, Environment);

			Environment.SetDefine(TEXT("USE_CACHE"), 1);
		}
	};

	IMPLEMENT_GLOBAL_SHADER(FKuwaharaFilterCS, "/AnimepoyShaders/Private/PostProcessKuwaharaFilter.usf", "KuwaharaFilterCS", SF_Compute);

	class FKuwaharaFilterPS : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FKuwaharaFilterPS);
		SHADER_USE_PARAMETER_STRUCT(FKuwaharaFilterPS, FGlobalShader);

		using FPermutationDomain = FCommonDomain;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SummedAreaTableTexture)
			SHADER_PARAMETER(int32, FilterSize)
			RENDER_TARGET_BINDING_SLOTS()
			END_SHADER_PARAMETER_STRUCT()

			static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& Environment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Parameters, Environment);

			Environment.SetDefine(TEXT("USE_CACHE"), 0);
		}
	};

	IMPLEMENT_GLOBAL_SHADER(FKuwaharaFilterPS, "/AnimepoyShaders/Private/PostProcessKuwaharaFilter.usf", "KuwaharaFilterPS", SF_Pixel);

	EPixelFormat GetSummedAreaTablePixelFormat(EKuwaharaFilterTargetType TargetType)
	{
		switch (TargetType)
		{
		case EKuwaharaFilterTargetType::SceneColor: return PF_A32B32G32R32F;
		case EKuwaharaFilterTargetType::BaseColor: return PF_FloatRGBA;
		case EKuwaharaFilterTargetType::Normal: return PF_FloatRGBA;
		case EKuwaharaFilterTargetType::Material: return PF_FloatRGBA;
		default: return PF_Unknown;
		}
	}

	EValueType GetValueType(EKuwaharaFilterTargetType TargetType)
	{
		switch (TargetType)
		{
		case EKuwaharaFilterTargetType::SceneColor: return EValueType::Color;
		case EKuwaharaFilterTargetType::BaseColor: return EValueType::Color;
		case EKuwaharaFilterTargetType::Normal: return EValueType::Normal;
		case EKuwaharaFilterTargetType::Material: return EValueType::Material;
		default: return EValueType::Color;
		}
	}
}

void AddKuwaharaFilterPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FKuwaharaFilterInputs& Inputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "AnimeKuwaharaFilter");

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	FScreenPassTextureViewport Viewport(View.ViewRect);

	FCommonDomain PermutationVector{};
	PermutationVector.Set<FValueType>(GetValueType(Inputs.TargetType));

	FRDGTextureRef SummedAreaTable{};
	{
		FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(Inputs.Target->Desc.Extent, GetSummedAreaTablePixelFormat(Inputs.TargetType), FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV);
		SummedAreaTable = GraphBuilder.CreateTexture(Desc, TEXT("SummedAreaTable"));

		FKuwaharaFilterSetupCS::FParameters* Parameters = GraphBuilder.AllocParameters<FKuwaharaFilterSetupCS::FParameters>();
		Parameters->View = View.ViewUniformBuffer;
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->InputTexture = Inputs.Target;
		Parameters->OutSummedAreaTableTexture = GraphBuilder.CreateUAV(SummedAreaTable);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("KuwaharaFilterSetupCS"),
			TShaderMapRef<FKuwaharaFilterSetupCS>(ShaderMap, PermutationVector),
			Parameters,
			FComputeShaderUtils::GetGroupCount(Viewport.Rect.Size(), FIntPoint(16, 16))
		);
	}

	bool bUAV = (int)Inputs.Target->Desc.Flags & (int)TexCreate_UAV;
	bool bSRGB = (int)Inputs.Target->Desc.Flags & (int)TexCreate_SRGB;
	bool bUseCompute = bUAV && !bSRGB;

	if (bUseCompute)
	{
		FKuwaharaFilterCS::FParameters* Parameters = GraphBuilder.AllocParameters<FKuwaharaFilterCS::FParameters>();
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->SummedAreaTableTexture = SummedAreaTable;
		Parameters->FilterSize = Inputs.FilterSize;
		Parameters->OutTexture = GraphBuilder.CreateUAV(Inputs.Target);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("KuwaharaFilterCS"),
			TShaderMapRef<FKuwaharaFilterCS>(ShaderMap, PermutationVector),
			Parameters,
			FComputeShaderUtils::GetGroupCount(Viewport.Rect.Size(), FIntPoint(16, 16))
		);
	}
	else
	{
		FKuwaharaFilterPS::FParameters* Parameters = GraphBuilder.AllocParameters<FKuwaharaFilterPS::FParameters>();
		Parameters->Input = GetScreenPassTextureViewportParameters(Viewport);
		Parameters->SummedAreaTableTexture = SummedAreaTable;
		Parameters->FilterSize = Inputs.FilterSize;
		Parameters->RenderTargets[0] = FRenderTargetBinding(Inputs.Target, ERenderTargetLoadAction::ELoad);

		FPixelShaderUtils::AddFullscreenPass(
			GraphBuilder,
			ShaderMap,
			RDG_EVENT_NAME("KuwaharaFilterPS"),
			TShaderMapRef<FKuwaharaFilterPS>(ShaderMap, PermutationVector),
			Parameters,
			Viewport.Rect,
			TStaticBlendState<CW_RGB>::GetRHI());
	}
}
