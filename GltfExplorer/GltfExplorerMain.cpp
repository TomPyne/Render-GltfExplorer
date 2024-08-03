#include <Render/Render.h>
#include <Clock.h>
#include <SurfMath.h>

#include "backends/imgui_impl_win32.h"
#include "imgui.h"
#include "imgui_impl_render.h"

#include "Camera/FlyCamera.h"
#include "TextureLoader.h"
#include "Scene.h"

using namespace tpr;

static constexpr RenderFormat DepthFormat = RenderFormat::D32_FLOAT;

static struct
{
	uint32_t ScreenWidth = 0;
	uint32_t ScreenHeight = 0;
	FlyCamera Camera;

	RenderPtr<Texture_t> DepthTexture;
	RenderPtr<DepthStencilView_t> Dsv;

	SScene Scene;
} G;

struct SkyDome
{
	VertexBufferPtr VertexBuffer;
	IndexBufferPtr IndexBuffer;

	uint32_t IndexCount = 0;

	TexturePtr SkyTexture;
	ShaderResourceViewPtr Srv;

	GraphicsPipelineStatePtr Pso;

	void Load(const char* path)
	{
		SkyTexture = LoadHdrTextureFromFile(path);
		Srv = CreateTextureSRV(SkyTexture, RenderFormat::R32G32B32A32_FLOAT, TextureDimension::TEX2D, 1u, 1u);
		
		constexpr uint32_t stacks = 16;
		constexpr uint32_t slices = 32;
		constexpr u32 indexCount = slices * 6u + slices * (stacks - 2u) * 6u;
		constexpr u32 vertexCount = slices * (stacks - 1) + 2u;

		IndexCount = indexCount;

		static_assert(indexCount < uint16_t(0xfff7), "Too many indices");

		std::vector<float3> verts;
		std::vector<uint16_t> indices;

		verts.resize(vertexCount);
		indices.resize(indexCount);

		float3* vertIter = &verts.front();
		u16* idxIter = &indices.front();

		*vertIter++ = float3(0, 0.5f, 0);

		const float stacksRcp = 1.0f / float(stacks);
		const float slicesRcp = 1.0f / float(slices);

		for (u32 i = 0; i < stacks - 1; i++)
		{
			//const float u = float(i) * stacksRcp;
			const float phi = K_PI * float(i + 1) * stacksRcp;
			const float sinPhi = sinf(phi);
			const float cosPhi = cosf(phi);

			for (u32 j = 0; j < slices; j++)
			{
				//const float v = float(j) * slicesRcp;
				const float theta = 2.0f * K_PI * float(j) * slicesRcp;
				const float sinTheta = sinf(theta);
				const float cosTheta = cosf(theta);

				float3 pos = float3(sinPhi * cosTheta, cosPhi, sinPhi * sinTheta);

				*vertIter++ = pos * 0.5f;
			}
		}

		*vertIter++ = float3(0, -0.5f, 0);

		for (u32 i = 0; i < slices; i++)
		{
			*idxIter++ = (i + 1) % slices + 1;
			*idxIter++ = i + 1;
			*idxIter++ = 0;
			*idxIter++ = vertexCount - 1;
			*idxIter++ = i + slices * (stacks - 2) + 1;
			*idxIter++ = (i + 1) % slices + slices * (stacks - 2) + 1;
		}

		for (u32 j = 0; j < stacks - 2; j++)
		{
			u16 j0 = j * slices + 1;
			u16 j1 = (j + 1) * slices + 1;
			for (u32 i = 0; i < slices; i++)
			{
				u16 i0 = j0 + i;
				u16 i1 = j0 + (i + 1) % slices;
				u16 i2 = j1 + (i + 1) % slices;
				u16 i3 = j1 + i;

				*idxIter++ = i0;
				*idxIter++ = i1;
				*idxIter++ = i2;
				*idxIter++ = i2;
				*idxIter++ = i3;
				*idxIter++ = i0;
			}
		}

		VertexBuffer = CreateVertexBuffer(verts.data(), verts.size() * sizeof(float3));
		IndexBuffer = CreateIndexBuffer(indices.data(), indices.size() * sizeof(uint16_t));

		VertexShader_t vertexShader = CreateVertexShader("Shaders/SkyDome.hlsl");
		PixelShader_t pixelShader = CreatePixelShader("Shaders/SkyDome.hlsl");

		GraphicsPipelineTargetDesc sceneTargetDesc({ RenderView::BackBufferFormat }, { BlendMode::None() });
		tpr::GraphicsPipelineStateDesc psoDesc = {};
		psoDesc.RasterizerDesc(tpr::PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::FRONT)
			.DepthDesc(true, ComparisionFunc::LESS_EQUAL, DepthFormat)
			.TargetBlendDesc(sceneTargetDesc)
			.VertexShader(vertexShader)
			.PixelShader(pixelShader);

		static constexpr tpr::InputElementDesc meshLayout[] =
		{
			{ "POSITION", 0, tpr::RenderFormat::R32G32B32_FLOAT,0, 0, tpr::InputClassification::PER_VERTEX, 0 },
		};

		Pso = tpr::CreateGraphicsPipelineState(psoDesc, meshLayout, ARRAYSIZE(meshLayout));

		assert(Pso);
	}

} GSkyDome;

void ResizeScreen(uint32_t width, uint32_t height)
{
	width = Max(width, 1u);
	height = Max(height, 1u);

	if (G.ScreenWidth == width && G.ScreenHeight == height)
	{
		return;
	}

	G.ScreenWidth = width;
	G.ScreenHeight = height;

	G.Camera.Resize(G.ScreenWidth, G.ScreenHeight);

	{
		TextureCreateDesc depthDesc = {};
		depthDesc.Width = G.ScreenWidth;
		depthDesc.Height = G.ScreenHeight;
		depthDesc.Format = DepthFormat;
		depthDesc.Flags = RenderResourceFlags::DSV;
		depthDesc.DebugName = L"SceneDepth";

		G.DepthTexture = CreateTexture(depthDesc);
		assert(G.DepthTexture != Texture_t::INVALID);

		G.Dsv = CreateTextureDSV(G.DepthTexture, DepthFormat, TextureDimension::TEX2D, 1u);
		assert(G.Dsv != DepthStencilView_t::INVALID);
	}
}

void DrawUI()
{
	static bool bShowDemoWindow = false;
	static bool bShowTextureWindow = false;

	if (ImGui::Begin("GltfExplorer"))
	{
		ImGui::Checkbox("Show Demo Window", &bShowDemoWindow);
		ImGui::Checkbox("Show Textures", &bShowTextureWindow);
	}
	ImGui::End();

	if (bShowDemoWindow)
	{
		ImGui::ShowDemoWindow();
	}

	if (bShowTextureWindow)
	{
		if (ImGui::Begin("Textures", &bShowTextureWindow))
		{
			uint32_t idx = 0;
			float windowWidth = ImGui::GetContentRegionAvail().x;

			for (const STexture& tex : G.Scene.Textures)
			{				
				ImGui::Image((ImTextureID)tex.Srv.Get(), ImVec2(windowWidth * 0.25f, windowWidth * 0.25f));
				idx++;

				if (idx % 4 != 0)
					ImGui::SameLine();
			}
		}
		ImGui::End();
	}
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

enum RootSigSlots
{
	RS_VIEW_BUF,
	RS_MESH_BUF,
	RS_MAT_BUF,
	RS_SRV_TABLE,
	RS_COUNT,
};

int main()
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "Gltf Explorer", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "Gltf Explorer", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	RenderInitParams params;
#ifdef _DEBUG
	params.DebugEnabled = false;
#else
	params.DebugEnabled = false;
#endif

	params.RootSigDesc.Flags = RootSignatureFlags::ALLOW_INPUT_LAYOUT;
	params.RootSigDesc.Slots.resize(RS_COUNT);
	params.RootSigDesc.Slots[RS_VIEW_BUF] = RootSignatureSlot::CBVSlot(0, 0);
	params.RootSigDesc.Slots[RS_MESH_BUF] = RootSignatureSlot::CBVSlot(1, 0);
	params.RootSigDesc.Slots[RS_MAT_BUF] = RootSignatureSlot::CBVSlot(2, 0);
	params.RootSigDesc.Slots[RS_SRV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, tpr::RootSignatureDescriptorTableType::SRV);

	params.RootSigDesc.GlobalSamplers.resize(1);
	params.RootSigDesc.GlobalSamplers[0].AddressModeUVW(SamplerAddressMode::WRAP).FilterModeMinMagMip(SamplerFilterMode::LINEAR);

	if (!Render_Init(params))
	{
		Render_ShutDown();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	RenderViewPtr view = CreateRenderViewPtr((intptr_t)hwnd);

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplRender_Init(RenderView::BackBufferFormat);

	G.Scene = LoadSceneFromGlb("Assets/SunTemple.glb");

	GSkyDome.Load("Assets/evening_sky_8k.hdr");

	//G.Scene.Release();

	GraphicsPipelineTargetDesc sceneTargetDesc({ RenderView::BackBufferFormat }, { BlendMode::None() });

	// Precache PSOs
	for (const SMaterial& mat : G.Scene.Materials)
	{
		GetPSOForMaterial(mat, sceneTargetDesc, DepthFormat);
	}

	Clock clock{};

	G.Camera.SetView(float3{ -2, 6, -2 }, 0.0f, 45.0f);

	// Main loop
	bool bQuit = false;
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (bQuit == false && msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		clock.Tick();

		const float deltaSeconds = clock.GetDeltaSeconds();

		G.Camera.UpdateView(deltaSeconds);

		// Because we use multiple viewports it is more efficient to sync at the latest possible point
		view->Sync();

		Render_BeginFrame();

		{
			ImGui_ImplRender_NewFrame();

			ImGui_ImplWin32_NewFrame();

			ImGui::NewFrame();

			DrawUI();

			ImGui::Render();
		}

		ImRenderFrameData* frameData = ImGui_ImplRender_PrepareFrameData(ImGui::GetDrawData());

		Render_BeginRenderFrame();

		CommandListSubmissionGroup clGroup(CommandListType::GRAPHICS);

		CommandList* cl = clGroup.CreateCommandList();

		UploadBuffers(cl);

		cl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::PRESENT, ResourceTransitionState::RENDER_TARGET);
		cl->TransitionResource(G.DepthTexture, ResourceTransitionState::COMMON, ResourceTransitionState::DEPTH_WRITE);

		// Bind and clear targets
		{
			RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();

			constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.2f, 1.0f };

			cl->ClearRenderTarget(backBufferRtv, DefaultClearCol);
			cl->ClearDepth(G.Dsv, 1.0f);

			cl->SetRenderTargets(&backBufferRtv, 1, G.Dsv);
		}

		// Draw scene
		{
			struct ViewUniforms
			{
				matrix ViewProjectionMat;
				float3 CameraPos;
				float __pad;
			} viewUniforms;

			viewUniforms.ViewProjectionMat = G.Camera.GetView() * G.Camera.GetProjection();
			viewUniforms.CameraPos = G.Camera.GetPosition();

			cl->SetRootSignature();

			Viewport vp(G.ScreenWidth, G.ScreenHeight);

			cl->SetViewports(&vp, 1);
			cl->SetDefaultScissor();

			DynamicBuffer_t viewCB = CreateDynamicConstantBuffer(&viewUniforms, sizeof(viewUniforms));

			if (Render_IsBindless())
			{
				cl->SetGraphicsRootCBV(RS_VIEW_BUF, viewCB);

				cl->SetGraphicsRootDescriptorTable(RS_SRV_TABLE);
			}
			else
			{
				cl->BindVertexCBVs(0, 1, &viewCB);
				cl->BindPixelCBVs(0, 1, &viewCB);
			}

			int count = 0;
			for (const SNode& node : G.Scene.Nodes)
			{
				if (node.Model == SceneModel_t::INVALID)
					continue;

				DynamicBuffer_t meshBuf = CreateDynamicConstantBuffer(&node.Transform, sizeof(node.Transform));
				if (Render_IsBindless())
				{
					cl->SetGraphicsRootCBV(RS_MESH_BUF, meshBuf);
				}
				else
				{
					cl->BindVertexCBVs(0, 1, &meshBuf);
				}

				const SModel& model = G.Scene.Models[(uint32_t)node.Model];
				for (const SMesh& mesh : model.Meshes)
				{
					if (mesh.Material == SceneMaterial_t::INVALID)
						continue;

					const SMaterial& material = G.Scene.Materials[(uint32_t)mesh.Material];

					cl->SetPipelineState(GetPSOForMaterial(material, sceneTargetDesc, DepthFormat));

					if (Render_IsBindless())
					{
						cl->SetGraphicsRootCBV(RS_MAT_BUF, material.ConstantBuffer);
					}
					else
					{
						cl->BindPixelCBVs(1, 1, &material.ConstantBuffer);

						cl->BindPixelSRVs(0, ARRAYSIZE(material.Srvs), material.Srvs);
					}

					cl->SetIndexBuffer(mesh.IndexBuffer, mesh.IndexFormat, mesh.IndexOffset);
					cl->SetVertexBuffers(0, ARRAYSIZE(mesh.VertexBuffersRaw), mesh.VertexBuffersRaw, mesh.BufferStrides, mesh.BufferOffsets);

					cl->DrawIndexedInstanced(mesh.IndexCount, 1, 0, 0, 0);

					count++;
				}
			}
		}

		{
			Viewport vp(G.ScreenWidth, G.ScreenHeight);
			vp.maxDepth = vp.minDepth = 1.0f;

			cl->SetViewports(&vp, 1);

			cl->SetPipelineState(GSkyDome.Pso);

			cl->SetIndexBuffer(GSkyDome.IndexBuffer, RenderFormat::R16_UINT, 0u);
			cl->SetVertexBuffer(0, GSkyDome.VertexBuffer, 12, 0);

			if (Render_IsBindless())
			{
				struct SkyDomeData
				{
					uint32_t SkySrv;
					float _pad[3];
				} skyData;

				skyData.SkySrv = GetDescriptorIndex(GSkyDome.Srv);

				DynamicBuffer_t skyDomeCbuf = CreateDynamicConstantBuffer(&skyData, sizeof(skyData));

				cl->SetGraphicsRootCBV(RS_MAT_BUF, skyDomeCbuf);
			}
			else
			{
				cl->BindPixelSRVs(0, 1, &GSkyDome.Srv);
			}


			cl->DrawIndexedInstanced(GSkyDome.IndexCount, 1, 0, 0, 0);
		}

		{
			cl->SetRootSignature(ImGui_ImplRender_GetRootSignature());

			ImGui_ImplRender_RenderDrawData(frameData, ImGui::GetDrawData(), cl);

			ImGui_ImplRender_ReleaseFrameData(frameData);
		}

		cl->TransitionResource(G.DepthTexture, ResourceTransitionState::DEPTH_WRITE, ResourceTransitionState::COMMON);
		cl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::RENDER_TARGET, ResourceTransitionState::PRESENT);

		clGroup.Submit();

		Render_EndFrame();

		view->Present(true, false);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	ImGui_ImplRender_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Render_ShutDown();

	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	RenderView* rv = GetRenderViewForHwnd((intptr_t)hWnd);

	switch (msg)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			const int w = (int)LOWORD(lParam);
			const int h = (int)HIWORD(lParam);

			if (rv)	rv->Resize(w, h);
			ResizeScreen(w, h);
			return 0;
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
