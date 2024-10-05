#pragma once

#include "SceneGraph/ISceneNode.h"

struct Gltf;
struct GltfLoadContext;
struct GltfMesh;
struct matrix;
struct SceneMaterialInstance;

struct StaticMeshPrimitive
{
	MeshBuffers Buffers;
	std::shared_ptr<SceneMaterialInstance> Material = nullptr;
};

class StaticMeshNode : public ISceneNode
{
public:

	StaticMeshNode(const matrix& nodeTransform, const std::vector<StaticMeshPrimitive>& primitives);

	uint32_t GetPrimitiveCount() const noexcept { return (uint32_t)MeshPrimitives.size(); }

	virtual void Draw(const RenderInfo& info, Renderables& renderables) const noexcept override;

protected:

	std::vector<StaticMeshPrimitive> MeshPrimitives;
};