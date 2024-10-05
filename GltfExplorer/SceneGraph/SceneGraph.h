#pragma once

#include "ISceneNode.h"

#include <SurfMath.h>

//struct SceneDirectionalLight
//{
//	float Theta = 0.2f;
//	float Phi = 0.0f;
//
//	float3 Radiance = float3{ 34.0f, 22.75f, 8.75f };
//
//	float3 ToCartesian() const
//	{
//		const float sinTheta = sinf(Theta);
//		return float3{
//			sinTheta * cosf(Phi),
//			sinTheta * sinf(Phi),
//			cosf(Theta)
//		};
//	}
//};

struct SceneGlobals
{
	/*SceneDirectionalLight DirectionalLight = {};*/
} GScene;

struct SceneView
{
	matrix ProjectionMatrix;
	matrix ViewMatrix;
	float3 CameraPosition;
	uint32_t ViewportWidth;
	uint32_t ViewportHeight;
};

struct SceneGraph
{
	void AddNode(const SceneNodePtr& node);

	void RemoveNode(const ISceneNode* node);

	void Update(float deltaSeconds);

	std::vector<Renderables> GatherRenderables(const SceneView& view) const;

private:

	std::vector<SceneNodePtr> Nodes;
};