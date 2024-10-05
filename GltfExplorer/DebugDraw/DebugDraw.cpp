#include "DebugDraw.h"

#include <Render/Render.h>

using namespace tpr;

struct DebugDrawGlobals_s
{
	bool Initialised = false;
	GraphicsPipelineStatePtr DebugDrawPSO = {};
} G;

constexpr u32 DebugDrawColor(const float4& v)
{
	const u8 r = (u8)(v.x * 255.0f);
	const u8 g = (u8)(v.y * 255.0f);
	const u8 b = (u8)(v.z * 255.0f);
	const u8 a = (u8)(v.w * 255.0f);
	return r << 24u | g << 16u | b << 8u | a;
}

constexpr u32 DebugDrawColor(const float3& v)
{
	return DebugDrawColor(float4(v, 1.0f));
}

void DebugDrawInit(const tpr::GraphicsPipelineTargetDesc& targetDesc)
{
	DebugDrawShutdown();

	G.Initialised = true;

	VertexShader_t vertexShader = CreateVertexShader("Shaders/DebugDraw.hlsl");
	PixelShader_t pixelShader = CreatePixelShader("Shaders/DebugDraw.hlsl");

	// Set up material
	GraphicsPipelineStateDesc psoDesc = {};
	psoDesc.RasterizerDesc(PrimitiveTopologyType::LINE, FillMode::SOLID, CullMode::FRONT)
		.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
		.TargetBlendDesc(targetDesc)
		.VertexShader(vertexShader)
		.PixelShader(pixelShader);

	constexpr tpr::InputElementDesc meshLayout[] =
	{
		{ "POSITION", 0, tpr::RenderFormat::R32G32B32_FLOAT,0, 0, tpr::InputClassification::PER_VERTEX, 0 },
		{ "COLOR", 0, tpr::RenderFormat::R8G8B8A8_UNORM, 0, 12, tpr::InputClassification::PER_VERTEX, 0 },
	};

	G.DebugDrawPSO = CreateGraphicsPipelineState(psoDesc, meshLayout, ARRAYSIZE(meshLayout));

	if (!G.DebugDrawPSO)
	{
		G.Initialised = false;
		return;
	}
}

void DebugDrawShutdown()
{
	G.DebugDrawPSO = {};

	G.Initialised = false;
}

void DebugDrawContext_s::DrawLine(const float3& a, const float3& b, u32 color)
{
	DebugDrawVertex_s* verts = AllocVerts(2u);

	verts[0].Position = a;
	verts[0].Color = color;

	verts[1].Position = b;
	verts[1].Color = color;
}

void DebugDrawContext_s::Render(tpr::CommandList* cl)
{
	if (!G.Initialised)
		return;

	if (Vertices.empty())
		return;

	DynamicBuffer_t vb = CreateDynamicVertexBuffer(Vertices.data(), Vertices.size() * sizeof(DebugDrawVertex_s));

	cl->SetPipelineState(G.DebugDrawPSO);

	cl->SetVertexBuffer(0, vb, sizeof(DebugDrawVertex_s), 0);

	cl->DrawInstanced(Vertices.size(), 1u, 0, 0);
}

DebugDrawVertex_s* DebugDrawContext_s::AllocVerts(size_t count)
{
	const size_t offset = Vertices.size();
	Vertices.resize(offset + count);
	return &Vertices[offset];
}
