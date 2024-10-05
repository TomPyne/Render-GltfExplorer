#include "SceneMaterial.h"

#include "SceneRendering.h"

#include <Render/Render.h>
#include <algorithm>

using namespace tpr;

std::vector<std::unique_ptr<SceneBaseMaterial>> GLoadedBaseMaterials;

//bool operator==(const InputElementDesc& lhs, const InputElementDesc& rhs)
//{
//	return strcmp(lhs.semanticName, rhs.semanticName) == 0 &&
//		lhs.semanticIndex == rhs.semanticIndex &&
//		lhs.format == rhs.format &&
//		lhs.inputSlot == rhs.inputSlot &&
//		lhs.alignedByteOffset == rhs.alignedByteOffset &&
//		lhs.inputSlotClass == rhs.inputSlotClass &&
//		lhs.instanceDataStepRate == rhs.instanceDataStepRate;
//}

SceneBaseMaterial* Scene_GetMaterialBase(const std::string& vertexShaderPath, const std::string& pixelShaderPath, const std::vector<InputElementDesc>& inputDesc, SceneMaterialDomain domain, bool doubleSided)
{
	auto it = std::find_if(GLoadedBaseMaterials.begin(), GLoadedBaseMaterials.end(), [&](const std::unique_ptr<SceneBaseMaterial>& mat)
	{
		return mat->DoubleSided == doubleSided &&
			mat->Domain == domain &&
			mat->VertexShaderPath == vertexShaderPath &&
			mat->PixelShaderPath == pixelShaderPath &&
			mat->InputDesc == inputDesc;			
	});

	if (it != GLoadedBaseMaterials.end())
	{
		return it->get();
	}

	GLoadedBaseMaterials.emplace_back(std::make_unique<SceneBaseMaterial>(vertexShaderPath, pixelShaderPath, inputDesc, domain, doubleSided));

	return nullptr;
}

SceneBaseMaterial::SceneBaseMaterial(const std::string& vertexShaderPath, const std::string& pixelShaderPath, const std::vector<InputElementDesc>& inputDesc, SceneMaterialDomain domain, bool doubleSided)
	: VertexShaderPath(vertexShaderPath)
	, PixelShaderPath(pixelShaderPath)
	, InputDesc(inputDesc)
	, Domain(domain)
	, DoubleSided(doubleSided)
{
	GraphicsPipelineTargetDesc opaqueTargetDesc({ SceneRenderSettings::ColorFormat }, { BlendMode::None() }, SceneRenderSettings::DepthFormat);
	GraphicsPipelineTargetDesc translucentTargetDesc({ SceneRenderSettings::ColorFormat }, { BlendMode::Default() }, SceneRenderSettings::DepthFormat);
	GraphicsPipelineTargetDesc shadowTargetDesc( {}, { BlendMode::None() }, SceneRenderSettings::ShadowDepthFormat);

	const CullMode cullMode = DoubleSided ? CullMode::NONE : CullMode::BACK;
	const CullMode shadowCullMode = DoubleSided ? CullMode::NONE : CullMode::FRONT;

	ShaderMacros macros = {};
	macros.reserve(6);

	macros.emplace_back("MAT_BM_OPAQUE", (uint32_t)SceneMaterialDomain::MD_OPAQUE);
	macros.emplace_back("MAT_BM_MASKED", (uint32_t)SceneMaterialDomain::MD_MASKED);
	macros.emplace_back("MAT_BM_TRANSLUCENT", (uint32_t)SceneMaterialDomain::MD_TRANSLUCENT);

	macros.emplace_back("MAT_BM", (uint32_t)Domain);
	macros.emplace_back("MAT_TWOSIDED", (uint32_t)DoubleSided);

	VertexShader_t vertexShader = CreateVertexShader(vertexShaderPath.c_str(), macros);
	PixelShader_t pixelShader = CreatePixelShader(pixelShaderPath.c_str(), macros);

	macros.push_back("MAT_SHADOW_PASS");
	VertexShader_t shadowVertexShader = CreateVertexShader(vertexShaderPath.c_str(), macros);
	PixelShader_t shadowPixelShader = domain == SceneMaterialDomain::MD_MASKED ? CreatePixelShader(pixelShaderPath.c_str(), macros) : PixelShader_t::INVALID;

	if (domain == SceneMaterialDomain::MD_OPAQUE || domain == SceneMaterialDomain::MD_MASKED)
	{
		GraphicsPipelineStateDesc psoDesc = {};
		psoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, cullMode)
			.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
			.TargetBlendDesc(opaqueTargetDesc)
			.VertexShader(vertexShader)
			.PixelShader(pixelShader);

		Pso = CreateGraphicsPipelineState(psoDesc, inputDesc.data(), inputDesc.size());

		psoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, shadowCullMode)
			.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
			.TargetBlendDesc(shadowTargetDesc)
			.VertexShader(shadowVertexShader)
			.PixelShader(shadowPixelShader);

		DepthPso = CreateGraphicsPipelineState(psoDesc, inputDesc.data(), inputDesc.size());
	}
	else if (domain == SceneMaterialDomain::MD_TRANSLUCENT)
	{
		GraphicsPipelineStateDesc psoDesc = {};
		psoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, cullMode)
			.DepthDesc(false, ComparisionFunc::LESS_EQUAL)
			.TargetBlendDesc(translucentTargetDesc)
			.VertexShader(vertexShader)
			.PixelShader(pixelShader);

		Pso = CreateGraphicsPipelineState(psoDesc, inputDesc.data(), inputDesc.size());
	}
}
