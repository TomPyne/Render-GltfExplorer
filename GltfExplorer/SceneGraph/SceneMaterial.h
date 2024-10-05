#pragma once

#include <Render/RenderTypes.h>
#include <SurfMath.h>
#include <map>
#include <shared_mutex>

enum class SceneMaterialDomain
{
	MD_OPAQUE,
	MD_MASKED,
	MD_TRANSLUCENT,
	MD_COUNT,
};

struct SceneBaseMaterial
{
	std::string VertexShaderPath;
	std::string PixelShaderPath;
	std::vector<tpr::InputElementDesc> InputDesc;
	SceneMaterialDomain Domain;
	bool DoubleSided;
	bool SupportsDepthOnly;

	tpr::GraphicsPipelineStatePtr Pso;
	tpr::GraphicsPipelineStatePtr DepthPso;

	SceneBaseMaterial(
		const std::string& vertexShaderPath, 
		const std::string& pixelShaderPath,
		const std::vector<tpr::InputElementDesc>& inputDesc, 
		SceneMaterialDomain domain, 
		bool doubleSided);
};

struct SceneMaterialConstants
{
	float4 BaseColorFactor;

	float3 EmissiveFactor;
	float MaskAlphaCutoff;

	uint32_t BaseColorTextureIndex;
	uint32_t NormalTextureIndex;
	uint32_t MetallicRoughnessTextureIndex;
	uint32_t EmissiveTextureIndex;

	uint32_t BaseColorUVIndex;
	uint32_t NormalUVIndex;
	uint32_t MetallicRoughnessUVIndex;
	uint32_t EmissiveUVIndex;

	float MetallicFactor;
	float RoughnessFactor;
	float __pad[2];
};

struct SceneMaterial : std::enable_shared_from_this<SceneMaterial>
{
	SceneBaseMaterial* MaterialBase = nullptr;

	enum Textures : uint32_t
	{
		BASE_COLOR_TEX,
		NORMAL_TEX,
		METALLIC_ROUGHNESS_TEX,
		EMISSIVE_TEX,
		COUNT,
	};
	static constexpr uint32_t kMaterialTextureCount = static_cast<uint32_t>(Textures::COUNT);

	tpr::TexturePtr Textures[kMaterialTextureCount] = {};
	tpr::ShaderResourceViewPtr Srvs[kMaterialTextureCount] = {};

	SceneMaterialConstants Constants = {};
	tpr::ConstantBufferPtr ConstantBuffer = {};
};

using SceneMaterialPtr = std::shared_ptr<SceneMaterial>;

SceneBaseMaterial* Scene_GetMaterialBase(const std::string& vertexShaderPath, const std::string& pixelShaderPath, const std::vector<tpr::InputElementDesc>& inputDesc, SceneMaterialDomain domain, bool doubleSided);

struct SceneShaderPair
{
	tpr::VertexShader_t VertexShader = {};
	tpr::PixelShader_t PixelShader = {};
};

struct SceneMaterialMap
{
	virtual ~SceneMaterialMap() = default;

	std::map<uint64_t, SceneShaderPair> CompiledShaders;
	std::map<uint64_t, tpr::GraphicsPipelineStatePtr> CompiledPipelines;

	template<typename T>
	inline static void HashCombine(uint64_t& hash, const T& value)
	{
		std::hash<T> h;
		hash ^= h(value) + 0x9e3779b9 + (hash << 9) + (hash >> 2);
	}

	std::shared_mutex Lock;
};