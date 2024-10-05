#pragma once

#include <Render/RenderTypes.h>
#include <SurfMath.h>
#include <vector>

struct DebugDrawVertex_s
{
	float3 Position;
	u32 Color;
};

struct DebugDrawContext_s
{
	void DrawLine(const float3& a, const float3& b, u32 color);

	void Render(tpr::CommandList* cl);

private:

	std::vector<DebugDrawVertex_s> Vertices;

	DebugDrawVertex_s* AllocVerts(size_t count);
};

constexpr u32 DebugDrawColor(const float4& v);
constexpr u32 DebugDrawColor(const float3& v);

void DebugDrawInit(const tpr::GraphicsPipelineTargetDesc& targetDesc);
void DebugDrawShutdown();