#pragma once

#include <cstdint>
#include <SurfMath.h>
#include <Render/RenderTypes.h>
#include <Render/Buffers.h>
#include <vector>

enum class MeshVertexBuffers : uint8_t
{
	POSITION,
	NORMAL,
	TANGENT,
	TEXCOORD0,
	TEXCOORD1,
	COUNT,
};

enum class SceneRenderPass : uint8_t
{
	SHADOW_PASS,
	OPAQUE_PASS,	
	TRANSLUCENT_PASS,
	SKYBOX_PASS,
	COUNT
};

enum class RenderPassSorting : uint8_t
{
	NONE,
	BACK_TO_FRONT,
	FRONT_TO_BACK,
	COUNT,
};

static const RenderPassSorting PassSortingMethod[(uint8_t)SceneRenderPass::COUNT] = {
	RenderPassSorting::NONE,			// SHADOW_PASS
	RenderPassSorting::FRONT_TO_BACK,	// OPAQUE_PASS
	RenderPassSorting::BACK_TO_FRONT,	// TRANSLUCENT_PASS
};

extern struct SceneRenderSettings
{
	static tpr::RenderFormat ColorFormat;
	static tpr::RenderFormat DepthFormat;
	static tpr::RenderFormat ShadowDepthFormat;
} GSceneRenderSettings;

struct PassInfo
{
	SceneRenderPass Pass;
	matrix Transform;
	float3 CameraPosition;
	tpr::Viewport Viewport;
};

struct RenderInfo
{
	PassInfo Pass[(uint8_t)SceneRenderPass::COUNT];
};

//struct SceneDirectionalLight
//{
//	float3 Direction;
//	float3 Radiance;
//};

struct MeshBuffers
{
	tpr::VertexBufferPtr VertexBuffers[(uint8_t)MeshVertexBuffers::COUNT];

	tpr::VertexBuffer_t BindVertexBuffers[(uint8_t)MeshVertexBuffers::COUNT];
	uint32_t Strides[(uint8_t)MeshVertexBuffers::COUNT];
	uint32_t Offsets[(uint8_t)MeshVertexBuffers::COUNT];

	tpr::IndexBuffer_t IndexBuffer;
	tpr::RenderFormat IndexFormat;
	uint32_t IndexCount;
};

struct ResourceBindings
{
	tpr::ShaderResourceView_t* VertexSrvBinds = nullptr;
	size_t VertexSrvCount = 0;
	tpr::ShaderResourceView_t* PixelSrvBinds = nullptr;
	size_t PixelSrvCount = 0;
};

struct RenderBatch
{
	tpr::GraphicsPipelineState_t PSO;
	tpr::DynamicBuffer_t BatchCBuf;
	tpr::ConstantBuffer_t MaterialCBuf;
	float ZDepth;

	const MeshBuffers* BufferBindings;
	const ResourceBindings* ResourceBindings;
};

struct Renderables
{
	void AddBatch(SceneRenderPass pass, const RenderBatch& batch);

private:
	std::vector<RenderBatch> PassBatches[(uint8_t)SceneRenderPass::COUNT];

	friend struct SceneRenderer;
};

struct SceneRenderer
{
	explicit SceneRenderer();

	void Render(tpr::CommandList* cl, Renderables* renderables, size_t numRenderables, const RenderInfo& info);
private:

	tpr::RootSignaturePtr RootSignature;
};