#pragma once

#include <cstdint>
#include <vector>

#include <Render/Render.h>
#include <SurfMath.h>

enum class EMeshVertexBuffers : uint32_t
{
    VB_POSITION,
    VB_NORMAL,
    VB_TANGENT,
    VB_TEXCOORD0,
    VB_TEXCOORD1,
    VB_COUNT,
};

enum class EMaterialDomain : uint32_t
{
    MD_OPAQUE,
    MD_MASKED,
    MD_TRANSLUCENT,
    MD_COUNT,
};

enum class SceneMaterial_t : uint32_t { INVALID };
enum class SceneModel_t : uint32_t { INVALID };

constexpr uint32_t KMeshVertexBufferCount = (uint32_t)EMeshVertexBuffers::VB_COUNT;

struct SMesh
{
    tpr::VertexBufferPtr VertexBuffers[KMeshVertexBufferCount] = {};
    tpr::VertexBuffer_t VertexBuffersRaw[KMeshVertexBufferCount] = {}; // For binding as array
    uint32_t BufferStrides[KMeshVertexBufferCount] = {};
    uint32_t BufferOffsets[KMeshVertexBufferCount] = {};

    tpr::IndexBufferPtr IndexBuffer = {};
    tpr::RenderFormat IndexFormat = tpr::RenderFormat::UNKNOWN;
    uint32_t IndexCount = 0u;
    uint32_t IndexOffset = 0u;

    SceneMaterial_t Material = SceneMaterial_t::INVALID;
};

struct SModel
{
    std::vector<SMesh> Meshes;
};

struct SMaterialConstants
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

enum class EMaterialTextures : uint32_t
{
    MT_BASE_COLOR,
    MT_NORMAL,
    MT_METALLIC_ROUGHNESS,
    MT_EMISSIVE,
    MT_COUNT,
};

constexpr uint32_t kMaterialTextureCount = static_cast<uint32_t>(EMaterialTextures::MT_COUNT);

struct SMaterial
{
    EMaterialDomain Domain = EMaterialDomain::MD_OPAQUE;
    bool IsDoubleSided = false;

    tpr::ShaderResourceView_t Srvs[kMaterialTextureCount] = {};

    SMaterialConstants Constants = {};
    tpr::ConstantBufferPtr ConstantBuffer = tpr::ConstantBuffer_t::INVALID;
};

struct STexture
{
    tpr::TexturePtr Texture = tpr::Texture_t::INVALID;
    tpr::ShaderResourceViewPtr Srv = tpr::ShaderResourceView_t::INVALID;
};

struct SNode
{
    matrix Transform = {};
    SceneModel_t Model = SceneModel_t::INVALID;
};

struct SScene
{
    std::vector<SNode> Nodes;
    std::vector<SModel> Models;
    std::vector<SMaterial> Materials;
    std::vector<STexture> Textures;
};

SScene LoadSceneFromGlb(const char* glbPath);

tpr::GraphicsPipelineState_t GetPSOForMaterial(const SMaterial& material, const tpr::GraphicsPipelineTargetDesc& targetDesc);
