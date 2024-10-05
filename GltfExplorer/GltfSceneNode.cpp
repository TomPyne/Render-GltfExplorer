#include "GltfSceneNode.h"

#include "GltfLoader.h"

#include "SceneGraph/SceneMaterial.h"
#include "GltfSceneNodeFactory.h"

StaticMeshNode::StaticMeshNode(const matrix& nodeTransform, const std::vector<StaticMeshPrimitive>& primitives)
{
    RelativeTransform = nodeTransform;
}

void StaticMeshNode::Draw(const RenderInfo& info, Renderables& renderables) const noexcept
{

}


