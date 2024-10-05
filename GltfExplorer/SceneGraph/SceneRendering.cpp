#include "SceneRendering.h"

#include <Render/Render.h>
#include <algorithm>

using namespace tpr;

SceneRenderSettings GSceneRenderSettings;

tpr::RenderFormat SceneRenderSettings::ColorFormat = RenderFormat::R8G8B8A8_UNORM;
tpr::RenderFormat SceneRenderSettings::DepthFormat = RenderFormat::D32_FLOAT;
tpr::RenderFormat SceneRenderSettings::ShadowDepthFormat = RenderFormat::D32_FLOAT;

enum SceneRenderingRootSigSlots
{
	RS_VIEW_BUF,
	RS_BATCH_BUF,
	RS_MAT_BUF,
	RS_SRV_TABLE,
	RS_COUNT
};

void Renderables::AddBatch(SceneRenderPass pass, const RenderBatch& batch)
{
	PassBatches[(uint8_t)pass].push_back(batch);
}

SceneRenderer::SceneRenderer()
{
	RootSignatureDesc rootSigDesc = {};
	rootSigDesc.Flags = RootSignatureFlags::ALLOW_INPUT_LAYOUT;
	rootSigDesc.Slots.resize(RS_COUNT);
	rootSigDesc.Slots[RS_VIEW_BUF] = RootSignatureSlot::CBVSlot(0, 0);
	rootSigDesc.Slots[RS_BATCH_BUF] = RootSignatureSlot::CBVSlot(1, 0);
	rootSigDesc.Slots[RS_MAT_BUF] = RootSignatureSlot::CBVSlot(2, 0);
	rootSigDesc.Slots[RS_SRV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, RootSignatureDescriptorTableType::SRV);

	rootSigDesc.GlobalSamplers.resize(1);
	rootSigDesc.GlobalSamplers[0].AddressModeUVW(SamplerAddressMode::WRAP).FilterModeMinMagMip(SamplerFilterMode::LINEAR);

	RootSignature = CreateRootSignature(rootSigDesc);
}

void SceneRenderer::Render(CommandList* cl, Renderables* renderables, size_t numRenderables, const RenderInfo& info)
{
	std::vector<RenderBatch> CombinedBatches[(uint8_t)SceneRenderPass::COUNT];

	for (size_t i = 0; i < numRenderables; i++)
	{
		for (uint8_t p = 0; p < (uint8_t)SceneRenderPass::COUNT; p++)
		{
			CombinedBatches[p].append_range(renderables[i].PassBatches[p]);
		}
	}

	for (uint8_t p = 0; p < (uint8_t)SceneRenderPass::COUNT; p++)
	{
		if (PassSortingMethod[p] == RenderPassSorting::NONE)
		{
			continue;
		}
		else if (PassSortingMethod[p] == RenderPassSorting::BACK_TO_FRONT)
		{
			std::sort(CombinedBatches[p].begin(), CombinedBatches[p].end(), [](const RenderBatch& a, const RenderBatch& b) { return a.ZDepth > b.ZDepth; });
		}
		else if (PassSortingMethod[p] == RenderPassSorting::FRONT_TO_BACK)
		{
			std::sort(CombinedBatches[p].begin(), CombinedBatches[p].end(), [](const RenderBatch& a, const RenderBatch& b) { return a.ZDepth < b.ZDepth; });
		}
	}

	cl->SetRootSignature(RootSignature);

	struct ViewUniforms
	{
		matrix ViewProjectionMat;
		float3 CameraPos;
		float __pad0;
		float3 SunDirection;
		float __pad1;
		float3 SunRadiance;
		float __pad2;
	} viewUniforms;
	
	//viewUniforms.SunDirection = DirectionalLight.Direction;
	//viewUniforms.SunRadiance = DirectionalLight.Radiance;

	for (uint8_t p = 0; p < (uint8_t)SceneRenderPass::COUNT; p++)
	{
		viewUniforms.CameraPos = info.Pass[p].CameraPosition;
		viewUniforms.ViewProjectionMat = info.Pass[p].Transform;

		DynamicBuffer_t viewBuf = CreateDynamicConstantBuffer(&viewUniforms, sizeof(viewUniforms));

		cl->SetViewports(&info.Pass[p].Viewport, 1);
		cl->SetDefaultScissor();

		auto SubmitBatchesBindless = [&](const std::vector<RenderBatch>& batches)
		{
			cl->SetGraphicsRootCBV(RS_VIEW_BUF, viewBuf);
			cl->SetGraphicsRootDescriptorTable(RS_SRV_TABLE);

			for (const RenderBatch& batch : CombinedBatches[p])
			{
				cl->SetPipelineState(batch.PSO);

				if (batch.BatchCBuf != DynamicBuffer_t::INVALID)
					cl->SetGraphicsRootCBV(RS_BATCH_BUF, batch.BatchCBuf);

				if (batch.MaterialCBuf != ConstantBuffer_t::INVALID)
					cl->SetGraphicsRootCBV(RS_MAT_BUF, batch.MaterialCBuf);

				const MeshBuffers& mesh = *batch.BufferBindings;
				cl->SetIndexBuffer(mesh.IndexBuffer, mesh.IndexFormat, 0u);
				cl->SetVertexBuffers(0, (uint32_t)MeshVertexBuffers::COUNT, mesh.BindVertexBuffers, mesh.Strides, mesh.Offsets);

				cl->DrawIndexedInstanced(mesh.IndexCount, 1, 0, 0, 0);
			}
		};

		auto SubmitBatches = [&](const std::vector<RenderBatch>& batches)
		{
			cl->BindVertexCBVs(0, 1, &viewBuf);
			cl->BindPixelCBVs(0, 1, &viewBuf);

			for (const RenderBatch& batch : CombinedBatches[p])
			{
				cl->SetPipelineState(batch.PSO);

				cl->BindVertexCBVs(0, 1, &batch.BatchCBuf);
				cl->BindPixelCBVs(0, 1, &batch.BatchCBuf);

				cl->BindVertexCBVs(0, 1, &batch.MaterialCBuf);
				cl->BindPixelCBVs(0, 1, &batch.MaterialCBuf);

				if (batch.ResourceBindings)
				{
					if (batch.ResourceBindings->VertexSrvCount != 0)
					{
						cl->BindVertexSRVs(0, batch.ResourceBindings->VertexSrvCount, batch.ResourceBindings->VertexSrvBinds);
					}

					if (batch.ResourceBindings->PixelSrvCount != 0)
					{
						cl->BindVertexSRVs(0, batch.ResourceBindings->PixelSrvCount, batch.ResourceBindings->PixelSrvBinds);
					}
				}

				const MeshBuffers& mesh = *batch.BufferBindings;
				cl->SetIndexBuffer(mesh.IndexBuffer, mesh.IndexFormat, 0u);
				cl->SetVertexBuffers(0, (uint32_t)MeshVertexBuffers::COUNT, mesh.BindVertexBuffers, mesh.Strides, mesh.Offsets);

				cl->DrawIndexedInstanced(mesh.IndexCount, 1, 0, 0, 0);
			}
		};

		if (Render_IsBindless())
		{
			SubmitBatchesBindless(CombinedBatches[p]);
		}
		else
		{
			SubmitBatches(CombinedBatches[p]);
		}
	}
}