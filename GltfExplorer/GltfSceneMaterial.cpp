#include "GltfSceneMaterial.h"

#include <Render/PipelineState.h>

#include <shared_mutex>

static std::unique_ptr<GltfSceneMaterial> Singleton;

static std::vector<tpr::InputElementDesc> GetInputDesc(bool hasNormal, bool hasTangent, bool hasTexcoord0, bool hasTexcoord1)
{
    std::vector<tpr::InputElementDesc> input =
    {
        input.emplace_back("POSITION",   0, tpr::RenderFormat::R32G32B32_FLOAT,      0, 0,   tpr::InputClassification::PER_VERTEX,   0)
    };
    if (hasNormal)
    {
        input.emplace_back("NORMAL", 0, tpr::RenderFormat::R32G32B32_FLOAT, 1, 0, tpr::InputClassification::PER_VERTEX, 0);
    }
    if (hasTangent)
    {
        input.emplace_back("TANGENT", 0, tpr::RenderFormat::R32G32B32A32_FLOAT, 2, 0, tpr::InputClassification::PER_VERTEX, 0);
    }
    if (hasTexcoord0)
    {
        input.emplace_back("TEXCOORD", 0, tpr::RenderFormat::R32G32_FLOAT, 3, 0, tpr::InputClassification::PER_VERTEX, 0);
    }
    if (hasTexcoord1)
    {
        input.emplace_back("TEXCOORD", 1, tpr::RenderFormat::R32G32_FLOAT, 4, 0, tpr::InputClassification::PER_VERTEX, 0);
    }
    return input;
}

static tpr::ShaderMacros GetShaderMacros(bool isTwoSided, SceneMaterialDomain domain)
{
    tpr::ShaderMacros macros = {};
    macros.reserve(5);

    macros.emplace_back("MAT_BM_OPAQUE", (uint32_t)SceneMaterialDomain::MD_OPAQUE);
    macros.emplace_back("MAT_BM_MASKED", (uint32_t)SceneMaterialDomain::MD_MASKED);
    macros.emplace_back("MAT_BM_TRANSLUCENT", (uint32_t)SceneMaterialDomain::MD_TRANSLUCENT);

    macros.emplace_back("MAT_BM", (uint32_t)domain);
    macros.emplace_back("MAT_TWOSIDED", (uint32_t)isTwoSided);

    return macros;
}

static uint64_t GetShaderHash(bool hasNormal, bool hasTangent, bool hasTexcoord0, bool hasTexcoord1, bool isTwoSided, SceneMaterialDomain domain, bool depthOnly)
{
    uint64_t hash = 0;
    SceneMaterialMap::HashCombine(hash, hasNormal);
    SceneMaterialMap::HashCombine(hash, hasTangent);
    SceneMaterialMap::HashCombine(hash, hasTexcoord0);
    SceneMaterialMap::HashCombine(hash, hasTexcoord1);
    SceneMaterialMap::HashCombine(hash, isTwoSided);
    SceneMaterialMap::HashCombine(hash, (uint32_t)domain);
    SceneMaterialMap::HashCombine(hash, depthOnly);
    return hash;
}

static uint64_t GetPipelineHash(uint64_t shaderHash, uint64_t targetHash)
{
    uint64_t hash = 0;
    SceneMaterialMap::HashCombine(hash, shaderHash);
    SceneMaterialMap::HashCombine(hash, targetHash);
    return hash;
}

static bool SupportsRenderPass(SceneMaterialDomain domain, bool isDepthOnly, const tpr::GraphicsPipelineTargetDesc& desc)
{
    uint32_t requiredRenderTargets = 1;
    if (isDepthOnly && domain != SceneMaterialDomain::MD_MASKED)
    {
        requiredRenderTargets = 0;
    }

    if (desc.NumRenderTargets < requiredRenderTargets)
    {
        return false;
    }

    // Todo validate render target component type is float and component count is 4
    return true;
}

tpr::GraphicsPipelineStatePtr GltfSceneMaterial::FindOrCreatePSO(bool hasNormal, bool hasTangent, bool hasTexcoord0, bool hasTexcoord1, bool isTwoSided, SceneMaterialDomain domain, bool depthOnly, const tpr::GraphicsPipelineTargetDesc& targetDesc)
{
    const uint64_t targetHash = targetDesc.Hash();
    const uint64_t shaderHash = GetShaderHash(hasNormal, hasTangent, hasTexcoord0, hasTexcoord1, isTwoSided, domain, depthOnly);
    const uint64_t pipelineHash = GetPipelineHash(shaderHash, targetHash);

    // Find pipeline
    {
        auto readLock = std::shared_lock(Lock);

        auto it = CompiledPipelines.find(pipelineHash);
        if (it != CompiledPipelines.end())
        {
            return it->second;
        }
    }

    auto writeLock = std::unique_lock(Lock);

    SceneShaderPair shaders = {};
    // No pipeline, find or create compiled shaders
    {
        auto it = CompiledShaders.find(shaderHash);
        if (it != CompiledShaders.end())
        {
            shaders = it->second;
        }
        else
        {
            tpr::ShaderMacros macros = GetShaderMacros(isTwoSided, domain);
            shaders.VertexShader = tpr::CreateVertexShader(ShaderPath, macros);
            shaders.PixelShader = tpr::CreatePixelShader(ShaderPath, macros);

            CompiledShaders.insert(std::make_pair(shaderHash, shaders));
        }
    }

    const std::vector<tpr::InputElementDesc> inputDesc = GetInputDesc(hasNormal, hasTangent, hasTexcoord0, hasTexcoord1);

    const tpr::CullMode cullMode = isTwoSided ? tpr::CullMode::NONE : tpr::CullMode::BACK;

    tpr::GraphicsPipelineStateDesc psoDesc = {};
    psoDesc.RasterizerDesc(tpr::PrimitiveTopologyType::TRIANGLE, tpr::FillMode::SOLID, cullMode)
        .DepthDesc(true, tpr::ComparisionFunc::LESS_EQUAL)
        .TargetBlendDesc(targetDesc)
        .VertexShader(shaders.VertexShader)
        .PixelShader(shaders.PixelShader);

    tpr::GraphicsPipelineStatePtr pso = tpr::CreateGraphicsPipelineState(psoDesc, inputDesc.data(), inputDesc.size());

    CompiledPipelines.emplace(std::make_pair(pipelineHash, pso));

    return pso;
}

GltfSceneMaterial& GltfSceneMaterial::Get()
{
    if (Singleton == nullptr)
    {
        Singleton = std::unique_ptr<GltfSceneMaterial>(new GltfSceneMaterial());
    }

    return *Singleton.get();
}

tpr::GraphicsPipelineStatePtr GltfSceneMaterial::GetPSO(bool hasNormal, bool hasTangent, bool hasTexcoord0, bool hasTexcoord1, bool isTwoSided, SceneMaterialDomain domain, bool depthOnly, const tpr::GraphicsPipelineTargetDesc& targetDesc)
{
    if (!SupportsRenderPass(domain, depthOnly, targetDesc))
    {
        // Warning?
        return {};
    }

    return Get().FindOrCreatePSO(hasNormal, hasTangent, hasTexcoord0, hasTexcoord1, isTwoSided, domain, depthOnly, targetDesc);

}
