#include "GltfSceneNodeFactory.h"

#include "GltfLoader.h"
#include "TextureLoader.h"
#include "GltfSceneNode.h"

#include "SceneGraph/SceneMaterial.h"

#include <Render/Render.h>
#include <ppl.h>

#define PARALLEL_LOAD (RENDER_THREAD_SAFE)

struct GltfStaticMeshLoadContext
{
    explicit GltfStaticMeshLoadContext(const Gltf& src);

    void Process();

private:

    void ProcessImages();
    void ProcessMaterials();
    void ProcessScenes();
    void ProcessNode(uint32_t nodeIndex, std::stack<matrix>& matrixStack);

    const Gltf& Src;

    std::vector<std::shared_ptr<SceneMaterial>> LoadedMaterials;
    std::vector<tpr::TexturePtr> LoadedTextures;
    std::vector<tpr::ShaderResourceViewPtr> LoadedSrvs;
};

GltfStaticMeshLoadContext::GltfStaticMeshLoadContext(const Gltf& src)
    : Src(src)
{
    LoadedMaterials.resize(Src.materials.size());
    LoadedTextures.resize(Src.images.size());
}

void GltfStaticMeshLoadContext::Process()
{
    ProcessImages();
    ProcessMaterials();
}

void GltfStaticMeshLoadContext::ProcessImages()
{
    LoadedTextures.resize(Src.images.size());

    // Gltf loads images as texture + sampler combos, im assuming trilinear always to simplify it

#if PARALLEL_LOAD
    Concurrency::parallel_for((size_t)0u, Src.images.size(), [&](size_t i)
#else
    for (uint32_t i = 0; i < Src.images.size(); i++)
#endif
    {
        const GltfImage& gltfImage = Src.images[i];

        const GltfBufferView& gltfBufView = Src.bufferViews[gltfImage.bufferView];

        LoadedTextures[i] = LoadTextureFromBinary(Src.data.get() + gltfBufView.byteOffset, gltfBufView.byteLength);
        LoadedSrvs[i] = tpr::CreateTextureSRV(LoadedTextures[i], tpr::RenderFormat::R8G8B8A8_UNORM, tpr::TextureDimension::TEX2D, 1u, 1u);
    }
#if PARALLEL_LOAD
    );
#endif
}

void GltfStaticMeshLoadContext::ProcessMaterials()
{
    LoadedMaterials.resize(Src.materials.size());

    static const std::string kShaderPath = "Shaders/GltfMesh.hlsl";

    for (uint32_t i = 0; i < Src.materials.size(); i++)
    {
        const GltfMaterial& gltfMaterial = Src.materials[i];
        SceneMaterial* material = LoadedMaterials[i].get();

        SceneMaterialDomain domain = SceneMaterialDomain::MD_OPAQUE;
        switch (gltfMaterial.alphaMode)
        {
        case GltfAlphaMode::OPAQUE: domain = SceneMaterialDomain::MD_OPAQUE; break;
        case GltfAlphaMode::MASK: domain = SceneMaterialDomain::MD_MASKED; break;
        case GltfAlphaMode::BLEND: domain = SceneMaterialDomain::MD_TRANSLUCENT; break;
        }

        auto GetSrvForTexInfo = [this]<typename T>(const std::optional<T>&texInfo)->tpr::ShaderResourceViewPtr
        {
            return texInfo.has_value() && (LoadedTextures.size() > texInfo->index) ? LoadedSrvs[Src.textures[texInfo->index].source] : tpr::ShaderResourceViewPtr{};
        };

        auto GetUVIndexForTexInfo = [this]<typename T>(const std::optional<T>&texInfo)->uint32_t
        {
            return texInfo.has_value() ? texInfo->texcoord : 0u;
        };

        material->Srvs[SceneMaterial::BASE_COLOR_TEX] = GetSrvForTexInfo(gltfMaterial.pbr.baseColorTexture);
        material->Srvs[SceneMaterial::NORMAL_TEX] = GetSrvForTexInfo(gltfMaterial.normalTexture);
        material->Srvs[SceneMaterial::METALLIC_ROUGHNESS_TEX] = GetSrvForTexInfo(gltfMaterial.pbr.metallicRoughnessTexture);
        material->Srvs[SceneMaterial::EMISSIVE_TEX] = GetSrvForTexInfo(gltfMaterial.emissiveTexture);

        material->Constants.BaseColorFactor = float4((float)gltfMaterial.pbr.baseColorFactor.x, (float)gltfMaterial.pbr.baseColorFactor.y, (float)gltfMaterial.pbr.baseColorFactor.z, (float)gltfMaterial.pbr.baseColorFactor.w);
        material->Constants.EmissiveFactor = float3((float)gltfMaterial.emissiveFactor.x, (float)gltfMaterial.emissiveFactor.y, (float)gltfMaterial.emissiveFactor.z);
        material->Constants.MaskAlphaCutoff = gltfMaterial.alphaCutoff;
        material->Constants.MetallicFactor = gltfMaterial.pbr.metallicFactor;
        material->Constants.RoughnessFactor = gltfMaterial.pbr.roughnessFactor;

        material->Constants.BaseColorTextureIndex = tpr::GetDescriptorIndex(material->Srvs[SceneMaterial::BASE_COLOR_TEX]);
        material->Constants.NormalTextureIndex = tpr::GetDescriptorIndex(material->Srvs[SceneMaterial::NORMAL_TEX]);
        material->Constants.MetallicRoughnessTextureIndex = tpr::GetDescriptorIndex(material->Srvs[SceneMaterial::METALLIC_ROUGHNESS_TEX]);
        material->Constants.EmissiveTextureIndex = tpr::GetDescriptorIndex(material->Srvs[SceneMaterial::EMISSIVE_TEX]);

        material->Constants.BaseColorUVIndex = GetUVIndexForTexInfo(gltfMaterial.pbr.baseColorTexture);
        material->Constants.NormalUVIndex = GetUVIndexForTexInfo(gltfMaterial.normalTexture);
        material->Constants.MetallicRoughnessUVIndex = GetUVIndexForTexInfo(gltfMaterial.pbr.metallicRoughnessTexture);
        material->Constants.EmissiveUVIndex = GetUVIndexForTexInfo(gltfMaterial.emissiveTexture);

        material->ConstantBuffer = tpr::CreateConstantBuffer(&material->Constants, sizeof(material->Constants));
    }
}

void GltfStaticMeshLoadContext::ProcessScenes()
{
    for (const GltfScene& scene : Src.scenes)
    {
        std::stack<matrix> transformStack;
        transformStack.push(MakeMatrixIdentity());

        for (const uint32_t nodeIdx : scene.nodes)
        {
            ProcessNode(nodeIdx, transformStack);
        }
    }
}

void GltfStaticMeshLoadContext::ProcessNode(uint32_t nodeIndex, std::stack<matrix>& matrixStack)
{
    const GltfNode& gltfNode = Src.nodes[nodeIndex];

    matrix transform = {};

    for (uint32_t r = 0; r < 4u; r++)
    {
        transform.r[r] = float4{
            static_cast<float>(gltfNode.matrix.m[r * 4u + 0u]),
            static_cast<float>(gltfNode.matrix.m[r * 4u + 1u]),
            static_cast<float>(gltfNode.matrix.m[r * 4u + 2u]),
            static_cast<float>(gltfNode.matrix.m[r * 4u + 3u])
        };
    }

    matrixStack.push(matrixStack.top() * transform);

    if (gltfNode.mesh >= 0)
    {
        const GltfMesh& gltfMesh = Src.meshes[gltfNode.mesh];

        bool validMesh = false;
        for (const GltfMeshPrimitive& gltfPrim : gltfMesh.primitives)
        {
            // Only support triangles for simplicity
            if (gltfPrim.mode != GltfMeshMode::TRIANGLES)
                continue;

            validMesh = true;
            break;
        }

        if (validMesh)
        {
            
        }
    }
}

std::vector<SceneNodePtr> CreateSceneNodes(const Gltf& gltf)
{

    return {};
}

void GltfStaticMeshNodeFactory::CreateSceneNodes(const Gltf& gltf, SceneGraph* scene)
{
    GltfStaticMeshLoadContext context = GltfStaticMeshLoadContext(gltf);

    context.Process();
}
