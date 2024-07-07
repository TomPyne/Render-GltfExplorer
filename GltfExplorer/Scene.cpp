#include "Scene.h"

#include "Logging.h"
#include "GltfLoader.h"
#include "TextureLoader.h"

#include <Render/Buffers.h>
#include <stack>

void SMesh::Release()
{
    for (uint32_t i = 0; i < KMeshVertexBufferCount; i++)
    {
        tpr::RenderRelease(VertexBuffers[i]);
        VertexBuffers[i] = tpr::VertexBuffer_t::INVALID;
    }

    tpr::RenderRelease(IndexBuffer);
    IndexBuffer = tpr::IndexBuffer_t::INVALID;
}

void SModel::Release()
{
    for (SMesh& mesh : Meshes)
    {
        mesh.Release();
    }

    Meshes.clear();
}

void SMaterial::Release()
{
    tpr::RenderRelease(ConstantBuffer);
    ConstantBuffer = tpr::ConstantBuffer_t::INVALID;
}

void STexture::Release()
{
    tpr::RenderRelease(Texture);
    tpr::RenderRelease(Srv);

    Texture = tpr::Texture_t::INVALID;
    Srv = tpr::ShaderResourceView_t::INVALID;
}

void SNode::Release()
{
}

void SScene::Release()
{
    for (SNode& node : Nodes)
    {
        node.Release();
    }
    Nodes.clear();

    for (SModel& model : Models)
    {
        model.Release();
    }
    Models.clear();

    for (SMaterial& material : Materials)
    {
        material.Release();
    }
    Materials.clear();

    for (STexture& texture : Textures)
    {
        texture.Release();
    }
    Textures.clear();
}

struct SGltfProcessor
{
    Gltf& GltfModel;
    SScene LoadedScene;

    SGltfProcessor(Gltf& _model) : GltfModel(_model) {}

    SScene Process()
    {
        LoadedScene.Release();

        LoadedScene.Materials.resize(1);
        LoadedScene.Models.resize(1);

        ProcessImages();
        ProcessMaterials();
        ProcessMeshes();                   

        for (const GltfScene& scene : GltfModel.scenes)
        {
            std::stack<matrix> transformStack;
            transformStack.push(MakeMatrixIdentity());

            for (const uint32_t nodeIdx : scene.nodes)
            {
                ProcessNode(nodeIdx, transformStack);
            }
        }

        return LoadedScene;
    }

private:

    template<typename T>
    tpr::ShaderResourceView_t GetSrvForTexInfo(const std::optional<T>& texInfo)
    {
        return texInfo.has_value() && (LoadedScene.Textures.size() > texInfo->index) ? LoadedScene.Textures[GltfModel.textures[texInfo->index].source].Srv : tpr::ShaderResourceView_t::INVALID;
    }

    template<typename T>
    uint32_t GetUVIndexForTexInfo(const std::optional<T>& texInfo)
    {
        return texInfo.has_value() ? texInfo->texcoord : 0u;
    }

    void ProcessImages()
    {
        LoadedScene.Textures.resize(GltfModel.images.size());

        // Gltf loads images as texture + sampler combos, im assuming trilinear always to simplify it
        for (uint32_t i = 0; i < GltfModel.images.size(); i++)
        {
            const GltfImage& gltfImage = GltfModel.images[i];
            
            const GltfBufferView& gltfBufView = GltfModel.bufferViews[gltfImage.bufferView];

            LoadedScene.Textures[i].Texture = LoadTextureFromBinary(GltfModel.data.get() + gltfBufView.byteOffset, gltfBufView.byteLength);
            LoadedScene.Textures[i].Srv = tpr::CreateTextureSRV(LoadedScene.Textures[i].Texture, tpr::RenderFormat::R8G8B8A8_UNORM, tpr::TextureDimension::TEX2D, 1u, 1u);
        }
    }

    void ProcessMaterials()
    {
        LoadedScene.Materials.resize(GltfModel.materials.size());

        for (uint32_t i = 0; i < GltfModel.materials.size(); i++)
        {
            const GltfMaterial& gltfMaterial = GltfModel.materials[i];
            SMaterial& material = LoadedScene.Materials[i];

            switch (gltfMaterial.alphaMode)
            {
            case GltfAlphaMode::OPAQUE: material.Domain = EMaterialDomain::MD_OPAQUE; break;
            case GltfAlphaMode::MASK: material.Domain = EMaterialDomain::MD_MASKED; break;
            case GltfAlphaMode::BLEND: material.Domain = EMaterialDomain::MD_TRANSLUCENT; break;
            }

            material.IsDoubleSided = gltfMaterial.doubleSided;

            material.Srvs[(uint32_t)EMaterialTextures::MT_BASE_COLOR] = GetSrvForTexInfo(gltfMaterial.pbr.baseColorTexture);
            material.Srvs[(uint32_t)EMaterialTextures::MT_NORMAL] = GetSrvForTexInfo(gltfMaterial.normalTexture);
            material.Srvs[(uint32_t)EMaterialTextures::MT_METALLIC_ROUGHNESS] = GetSrvForTexInfo(gltfMaterial.pbr.metallicRoughnessTexture);
            material.Srvs[(uint32_t)EMaterialTextures::MT_EMISSIVE] = GetSrvForTexInfo(gltfMaterial.emissiveTexture);

            material.Constants.BaseColorFactor = float4((float)gltfMaterial.pbr.baseColorFactor.x, (float)gltfMaterial.pbr.baseColorFactor.y, (float)gltfMaterial.pbr.baseColorFactor.z, (float)gltfMaterial.pbr.baseColorFactor.w);
            material.Constants.EmissiveFactor = float3((float)gltfMaterial.emissiveFactor.x, (float)gltfMaterial.emissiveFactor.y, (float)gltfMaterial.emissiveFactor.z);
            material.Constants.MaskAlphaCutoff = gltfMaterial.alphaCutoff;
            material.Constants.MetallicFactor = gltfMaterial.pbr.metallicFactor;
            material.Constants.RoughnessFactor = gltfMaterial.pbr.roughnessFactor;

            material.Constants.BaseColorTextureIndex = tpr::GetDescriptorIndex(material.Srvs[(uint32_t)EMaterialTextures::MT_BASE_COLOR]);
            material.Constants.NormalTextureIndex = tpr::GetDescriptorIndex(material.Srvs[(uint32_t)EMaterialTextures::MT_NORMAL]);
            material.Constants.MetallicRoughnessTextureIndex = tpr::GetDescriptorIndex(material.Srvs[(uint32_t)EMaterialTextures::MT_METALLIC_ROUGHNESS]);
            material.Constants.EmissiveTextureIndex = tpr::GetDescriptorIndex(material.Srvs[(uint32_t)EMaterialTextures::MT_EMISSIVE]);

            material.Constants.BaseColorUVIndex = GetUVIndexForTexInfo(gltfMaterial.pbr.baseColorTexture);
            material.Constants.NormalUVIndex = GetUVIndexForTexInfo(gltfMaterial.normalTexture);
            material.Constants.MetallicRoughnessUVIndex = GetUVIndexForTexInfo(gltfMaterial.pbr.metallicRoughnessTexture);
            material.Constants.EmissiveUVIndex = GetUVIndexForTexInfo(gltfMaterial.emissiveTexture);

            material.ConstantBuffer = tpr::CreateConstantBuffer(&material.Constants, sizeof(material.Constants));
        }
    }

    void ProcessMeshes()
    {
        LoadedScene.Models.resize(GltfModel.meshes.size());

        for (uint32_t modelIt = 0; modelIt < GltfModel.meshes.size(); modelIt++)
        {
            const GltfMesh& gltfMesh = GltfModel.meshes[modelIt];

            SModel& model = LoadedScene.Models[modelIt];

            model.Meshes.resize(gltfMesh.primitives.size());

            for (uint32_t meshIt = 0; meshIt < gltfMesh.primitives.size(); meshIt++)
            {
                const GltfMeshPrimitive& gltfPrim = gltfMesh.primitives[meshIt];

                SMesh& mesh = model.Meshes[meshIt];

                mesh.Material = (SceneMaterial_t)gltfPrim.material;

                // Load index buffer
                {
                    const GltfAccessor& gltfAccessor = GltfModel.accessors[gltfPrim.indices];
                    const GltfBufferView& gltfBufView = GltfModel.bufferViews[gltfAccessor.bufferView];

                    const size_t offset = gltfAccessor.byteOffset + gltfBufView.byteOffset;

                    mesh.IndexBuffer = tpr::CreateIndexBuffer(
                        GltfModel.data.get() + offset,
                        gltfAccessor.count * GltfLoader_SizeOfComponent(gltfAccessor.componentType) * GltfLoader_ComponentCount(gltfAccessor.type)
                    );

                    mesh.IndexCount = gltfAccessor.count;
                    mesh.IndexOffset = 0;
                    mesh.IndexFormat = GltfLoader_SizeOfComponent(gltfAccessor.componentType) == 2 ? tpr::RenderFormat::R16_UINT : tpr::RenderFormat::R32_UINT;
                }

                // Load vertex buffers
                {
                    for (const GltfMeshAttribute& gltfAttr : gltfPrim.attributes)
                    {
                        EMeshVertexBuffers targetBuffer = EMeshVertexBuffers::VB_COUNT;

                        if (gltfAttr.semantic == "POSITION") targetBuffer = EMeshVertexBuffers::VB_POSITION;
                        else if (gltfAttr.semantic == "NORMAL") targetBuffer = EMeshVertexBuffers::VB_NORMAL;
                        else if (gltfAttr.semantic == "TANGENT") targetBuffer = EMeshVertexBuffers::VB_TANGENT;
                        else if (gltfAttr.semantic == "TEXCOORD_0") targetBuffer = EMeshVertexBuffers::VB_TEXCOORD0;
                        else if (gltfAttr.semantic == "TEXCOORD_1") targetBuffer = EMeshVertexBuffers::VB_TEXCOORD1;
                        else
                        {
                            continue;
                        }

                        const GltfAccessor& gltfAccessor = GltfModel.accessors[gltfAttr.index];
                        const GltfBufferView& gltfBufView = GltfModel.bufferViews[gltfAccessor.bufferView];

                        const size_t stride = GltfLoader_SizeOfComponent(gltfAccessor.componentType) * GltfLoader_ComponentCount(gltfAccessor.type);

                        mesh.VertexBuffers[(uint32_t)targetBuffer] = tpr::CreateVertexBuffer(
                            GltfModel.data.get() + gltfAccessor.byteOffset + gltfBufView.byteOffset,
                            gltfAccessor.count * stride
                        );

                        mesh.BufferStrides[(uint32_t)targetBuffer] = static_cast<UINT>(stride);
                        mesh.BufferOffsets[(uint32_t)targetBuffer] = 0;
                    }
                }
            }
        }
    }

    void ProcessNode(uint32_t nodeIndex, std::stack<matrix>& matrixStack)
    {
        const GltfNode& gltfNode = GltfModel.nodes[nodeIndex];

        matrix transform = {};

        for (uint32_t r = 0; r < 4u; r++)
        {
            transform.r[r] = float4{
                static_cast<float>(gltfNode.matrix.m[r * 4u + 0u]),
                static_cast<float>(gltfNode.matrix.m[r * 4u + 1u]),
                static_cast<float>(gltfNode.matrix.m[r * 4u + 2u]),
                static_cast<float>(gltfNode.matrix.m[r * 4u + 3])
            };
        }

        matrixStack.push(matrixStack.top() * transform);

        if (gltfNode.mesh >= 0)
        {
            const GltfMesh& gltfMesh = GltfModel.meshes[gltfNode.mesh];

            for (const GltfMeshPrimitive& gltfPrim : gltfMesh.primitives)
            {
                // Only support triangles for simplicity
                if (gltfPrim.mode != GltfMeshMode::TRIANGLES)
                    continue;

                LoadedScene.Nodes.push_back({});
                SNode& node = LoadedScene.Nodes.back();

                node.Transform = matrixStack.top();

                node.Model = (SceneModel_t)gltfNode.mesh;
            }
        }

        for (const uint32_t child : gltfNode.children)
        {
            ProcessNode(child, matrixStack);
        }

        matrixStack.pop();
    }

};

SScene LoadSceneFromGlb(const char* glbPath)
{
    Gltf gltfModel;
    if (!GltfLoader_Load(glbPath, &gltfModel))
        return {};

    SGltfProcessor processor(gltfModel);

    return processor.Process();
}

tpr::GraphicsPipelineState_t GetPSOForMaterial(const SMaterial& material, const tpr::GraphicsPipelineTargetDesc& targetDesc, tpr::RenderFormat depthFormat)
{
    static tpr::GraphicsPipelineState_t Permutations[(uint32_t)EMaterialDomain::MD_COUNT][2] = {};

    tpr::GraphicsPipelineState_t& perm = Permutations[(uint32_t)material.Domain][(uint32_t)material.IsDoubleSided];
    if (perm != tpr::GraphicsPipelineState_t::INVALID)
    {
        return perm;
    }

    const tpr::CullMode cullMode = material.IsDoubleSided ? tpr::CullMode::NONE : tpr::CullMode::BACK;

    tpr::ShaderMacros macros = {};
    macros.reserve(5);

    macros.emplace_back("MAT_BM_OPAQUE", (uint32_t)EMaterialDomain::MD_OPAQUE);
    macros.emplace_back("MAT_BM_MASKED", (uint32_t)EMaterialDomain::MD_MASKED);
    macros.emplace_back("MAT_BM_TRANSLUCENT", (uint32_t)EMaterialDomain::MD_TRANSLUCENT);

    macros.emplace_back("MAT_BM", (uint32_t)material.Domain);
    macros.emplace_back("MAT_TWOSIDED", (uint32_t)material.IsDoubleSided);

    tpr::VertexShader_t vertexShader = tpr::CreateVertexShader("Shaders/GltfMesh.hlsl", macros);
    tpr::PixelShader_t pixelShader = tpr::CreatePixelShader("Shaders/GltfMesh.hlsl", macros);

    tpr::GraphicsPipelineStateDesc psoDesc = {};
    psoDesc.RasterizerDesc(tpr::PrimitiveTopologyType::TRIANGLE, tpr::FillMode::SOLID, cullMode)
        .DepthDesc(true, tpr::ComparisionFunc::LESS_EQUAL, depthFormat)
        .TargetBlendDesc(targetDesc)
        .VertexShader(vertexShader)
        .PixelShader(pixelShader);

    static constexpr tpr::InputElementDesc meshLayout[] =
    {
        { "POSITION",   0, tpr::RenderFormat::R32G32B32_FLOAT,      0, 0,   tpr::InputClassification::PER_VERTEX,   0 },
        { "NORMAL",     0, tpr::RenderFormat::R32G32B32_FLOAT,      1, 0,   tpr::InputClassification::PER_VERTEX,   0 },
        { "TANGENT",    0, tpr::RenderFormat::R32G32B32A32_FLOAT,   2, 0,   tpr::InputClassification::PER_VERTEX,   0 },
        { "TEXCOORD",   0, tpr::RenderFormat::R32G32_FLOAT,         3, 0,   tpr::InputClassification::PER_VERTEX,   0 },
        { "TEXCOORD",   1, tpr::RenderFormat::R32G32_FLOAT,         4, 0,   tpr::InputClassification::PER_VERTEX,   0 },
    };

    perm = tpr::CreateGraphicsPipelineState(psoDesc, meshLayout, ARRAYSIZE(meshLayout));

    if (perm == tpr::GraphicsPipelineState_t::INVALID)
    {
        LOGERROR("Failed to create a material PSO");
    }

    return perm;
}
