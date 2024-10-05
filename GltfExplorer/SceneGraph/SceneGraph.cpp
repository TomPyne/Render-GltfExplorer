#include "SceneGraph.h"

#include <algorithm>

void SceneGraph::AddNode(const SceneNodePtr& node)
{
    Nodes.push_back(node);
}

void SceneGraph::RemoveNode(const ISceneNode* node)
{
   Nodes.erase(
       std::remove_if(
           Nodes.begin(), 
           Nodes.end(), 
           [node](const SceneNodePtr& nodePtr) {return nodePtr.get() == node; }), 
       Nodes.end());
}

void SceneGraph::Update(float deltaSeconds)
{
    for (const SceneNodePtr& node : Nodes)
    {
        if (node->CanTick())
        {
            node->Update(deltaSeconds);
        }
    }
}

std::vector<Renderables> SceneGraph::GatherRenderables(const SceneView& view) const
{
    std::vector<Renderables> renderables;
    renderables.resize(1);

    const matrix camViewProjection = view.ViewMatrix * view.ProjectionMatrix;

    RenderInfo info;
    info.Pass[(uint8_t)SceneRenderPass::SHADOW_PASS].CameraPosition = view.CameraPosition;
    info.Pass[(uint8_t)SceneRenderPass::SHADOW_PASS].Pass = SceneRenderPass::SHADOW_PASS;
    info.Pass[(uint8_t)SceneRenderPass::SHADOW_PASS].Transform = {}; // TODO;
    info.Pass[(uint8_t)SceneRenderPass::SHADOW_PASS].Viewport = tpr::Viewport(view.ViewportWidth, view.ViewportHeight);

    info.Pass[(uint8_t)SceneRenderPass::OPAQUE_PASS].CameraPosition = view.CameraPosition;
    info.Pass[(uint8_t)SceneRenderPass::OPAQUE_PASS].Pass = SceneRenderPass::OPAQUE_PASS;
    info.Pass[(uint8_t)SceneRenderPass::OPAQUE_PASS].Transform = camViewProjection;
    info.Pass[(uint8_t)SceneRenderPass::OPAQUE_PASS].Viewport = tpr::Viewport(view.ViewportWidth, view.ViewportHeight);

    info.Pass[(uint8_t)SceneRenderPass::TRANSLUCENT_PASS].CameraPosition = view.CameraPosition;
    info.Pass[(uint8_t)SceneRenderPass::TRANSLUCENT_PASS].Pass = SceneRenderPass::TRANSLUCENT_PASS;
    info.Pass[(uint8_t)SceneRenderPass::TRANSLUCENT_PASS].Transform = camViewProjection;
    info.Pass[(uint8_t)SceneRenderPass::TRANSLUCENT_PASS].Viewport = tpr::Viewport(view.ViewportWidth, view.ViewportHeight);

    info.Pass[(uint8_t)SceneRenderPass::SKYBOX_PASS].CameraPosition = view.CameraPosition;
    info.Pass[(uint8_t)SceneRenderPass::SKYBOX_PASS].Pass = SceneRenderPass::SKYBOX_PASS;
    info.Pass[(uint8_t)SceneRenderPass::SKYBOX_PASS].Transform = camViewProjection;
    info.Pass[(uint8_t)SceneRenderPass::SKYBOX_PASS].Viewport = tpr::Viewport(view.ViewportWidth, view.ViewportHeight);
    info.Pass[(uint8_t)SceneRenderPass::SKYBOX_PASS].Viewport.minDepth = 1.0f;

    for (const SceneNodePtr& node : Nodes)
    {
        if (node->CanRender())
        {
            node->Draw(info, renderables[0]);
        }
    }

    return renderables;
}
