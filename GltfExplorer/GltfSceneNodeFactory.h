#pragma once

#include "SceneGraph/ISceneNode.h"

#include <stack>
#include <vector>

struct Gltf;
struct GltfSceneNode;
struct SceneGraph;
struct SceneMaterialInstance;

struct GltfStaticMeshNodeFactory
{
	static void CreateSceneNodes(const Gltf& gltf, SceneGraph* scene);
};
