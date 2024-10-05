#pragma once

#include "SceneGraph/SceneMaterial.h"
#include <Render/RenderTypes.h>

struct GltfSceneMaterial : public SceneMaterialMap
{
    virtual ~GltfSceneMaterial() = default;

protected:
    explicit GltfSceneMaterial() = default;

    tpr::GraphicsPipelineStatePtr FindOrCreatePSO(bool hasNormal, bool hasTangent, bool hasTexcoord0, bool hasTexcoord1, bool isTwoSided, SceneMaterialDomain domain, bool depthOnly, const tpr::GraphicsPipelineTargetDesc& targetDesc);

    static GltfSceneMaterial& Get();

    const char* ShaderPath = "Shaders/GltfMesh.hlsl";

public:
    static tpr::GraphicsPipelineStatePtr GetPSO(bool hasNormal, bool hasTangent, bool hasTexcoord0, bool hasTexcoord1, bool isTwoSided, SceneMaterialDomain domain, bool depthOnly, const tpr::GraphicsPipelineTargetDesc& targetDesc);
};