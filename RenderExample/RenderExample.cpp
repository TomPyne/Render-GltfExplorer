#include <Render/Render.h>
#include <SurfMath.h>

using namespace tpr;

static struct
{
	uint32_t screenWidth = 0;
	uint32_t screenHeight = 0;

	Texture_t depthTexture = Texture_t::INVALID;
	DepthStencilView_t depthDsv = DepthStencilView_t::INVALID;

	matrix projectionMatrix;
} G;

struct BindVertexBuffer
{
	VertexBuffer_t buf = VertexBuffer_t::INVALID;
	uint32_t stride = 0;
	uint32_t offset = 0;
};

struct BindIndexBuffer
{
	IndexBuffer_t buf = IndexBuffer_t::INVALID;
	RenderFormat format;
	uint32_t offset = 0;
	uint32_t count = 0;
};

struct Mesh
{
	BindVertexBuffer positionBuf;
	BindVertexBuffer colorBuf;
	BindIndexBuffer indexBuf;
};

Mesh CreateCubeMesh()
{
	Mesh mesh;

	constexpr float3 pftl = float3(-0.5f, 0.5f, 0.5f);
	constexpr float3 pftr = float3(0.5f, 0.5f, 0.5f);
	constexpr float3 pfbr = float3(0.5f, -0.5f, 0.5f);
	constexpr float3 pfbl = float3(-0.5f, -0.5f, 0.5f);

	constexpr float3 pbtl = float3(-0.5f, 0.5f, -0.5f);
	constexpr float3 pbtr = float3(0.5f, 0.5f, -0.5f);
	constexpr float3 pbbr = float3(0.5f, -0.5f, -0.5f);
	constexpr float3 pbbl = float3(-0.5f, -0.5f, -0.5f);

	float3 posVerts[6 * 4] =
	{
		pftl, pftr, pfbr, pfbl,
		pbtr, pbtl, pbbl, pbbr,
		pftr, pbtr, pbbr, pfbr,
		pbtl, pftl, pfbl, pbbl,
		pfbl, pfbr, pbbr, pbbl,
		pftl, pbtl, pbtr, pftr,
	};

	mesh.positionBuf.buf = CreateVertexBuffer(posVerts, sizeof(posVerts));
	mesh.positionBuf.offset = 0;
	mesh.positionBuf.stride = sizeof(float3);

	constexpr float3 cftl = (pftl + float3(1.0f)) * 0.5f;
	constexpr float3 cftr = (pftr + float3(1.0f)) * 0.5f;
	constexpr float3 cfbr = (pfbr + float3(1.0f)) * 0.5f;
	constexpr float3 cfbl = (pfbl + float3(1.0f)) * 0.5f;
								 			   	
	constexpr float3 cbtl = (pbtl + float3(1.0f)) * 0.5f;
	constexpr float3 cbtr = (pbtr + float3(1.0f)) * 0.5f;
	constexpr float3 cbbr = (pbbr + float3(1.0f)) * 0.5f;
	constexpr float3 cbbl = (pbbl + float3(1.0f)) * 0.5f;

	float3 colVerts[6 * 4] =
	{
		cftl, cftr, cfbr, cfbl,
		cbtr, cbtl, cbbl, cbbr,
		cftr, cbtr, cbbr, cfbr,
		cbtl, cftl, cfbl, cbbl,
		cfbl, cfbr, cbbr, cbbl,
		cftl, cbtl, cbtr, cftr,
	};

	mesh.colorBuf.buf = CreateVertexBuffer(colVerts, sizeof(colVerts));
	mesh.colorBuf.offset = 0;
	mesh.colorBuf.stride = sizeof(float3);

	u16 Indices[6 * 6] =
	{
		2, 1, 0, 0, 3, 2,
		6, 5, 4, 4, 7, 6,
		10, 9, 8, 8, 11, 10,
		14, 13, 12, 12, 15, 14,
		18, 17, 16, 16, 19, 18,
		22, 21, 20, 20, 23, 22,
	};

	mesh.indexBuf.buf = CreateIndexBuffer(Indices, sizeof(Indices));
	mesh.indexBuf.count = ARRAYSIZE(Indices);
	mesh.indexBuf.offset = 0;
	mesh.indexBuf.format = RenderFormat::R16_UINT;

	return mesh;
}

void ResizeScreen(uint32_t width, uint32_t height)
{
	width = Max(width, 1u);
	height = Max(height, 1u);

	if (G.screenWidth == width && G.screenHeight == height)
	{
		return;
	}

	G.screenWidth = width;
	G.screenHeight = height;

	RenderRelease(G.depthTexture);
	RenderRelease(G.depthDsv);

	TextureCreateDescEx depthDesc = {};
	depthDesc.DebugName = L"SceneDepth";
	depthDesc.Flags = RenderResourceFlags::DSV;
	depthDesc.ResourceFormat = RenderFormat::D16_UNORM;
	depthDesc.Height = G.screenHeight;
	depthDesc.Width = G.screenWidth;
	depthDesc.InitialState = ResourceTransitionState::DEPTH_WRITE;
	depthDesc.Dimension = TextureDimension::TEX2D;
	G.depthTexture = CreateTextureEx(depthDesc);
	G.depthDsv = CreateTextureDSV(G.depthTexture, RenderFormat::D16_UNORM, TextureDimension::TEX2D, 1u);

	constexpr float fovRad = ConvertToRadians(45.0f);

	float aspectRatio = (float)G.screenWidth / (float)G.screenHeight;
	
	G.projectionMatrix = MakeMatrixPerspectiveFovLH(fovRad, aspectRatio, 0.01f, 100.0f);
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main()
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "Render Example", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "Render Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	RenderInitParams params;
#ifdef _DEBUG
	params.DebugEnabled = true;
#else
	params.DebugEnabled = false;
#endif
	params.RootSigDesc.Flags = RootSignatureFlags::ALLOW_INPUT_LAYOUT;
	params.RootSigDesc.Slots.resize(2);
	params.RootSigDesc.Slots[0] = RootSignatureSlot::CBVSlot(0, 0);
	params.RootSigDesc.Slots[1] = RootSignatureSlot::CBVSlot(1, 0);

	if (!Render_Init(params))
	{
		Render_ShutDown();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	RenderViewPtr view = CreateRenderViewPtr((intptr_t)hwnd);

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	Mesh mesh = CreateCubeMesh();

	matrix viewMatrix = MakeMatrixLookAtLH(float3{ 0.0f, 0.0f, -2.0f }, float3{ 0.0f, 0.0f, 0.0f }, float3(0.0f, 1.0f, 0.0f));

	GraphicsPipelineState_t meshPSO = GraphicsPipelineState_t::INVALID;

	{
		VertexShader_t meshVS = CreateVertexShader("Shaders/Mesh.hlsl");
		PixelShader_t meshPS = CreatePixelShader("Shaders/Mesh.hlsl");

		GraphicsPipelineStateDesc psoDesc = {};
		psoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(true, ComparisionFunc::LESS_EQUAL, RenderFormat::D16_UNORM)
			.TargetBlendDesc({ RenderFormat::R8G8B8A8_UNORM }, { BlendMode::Default() })
			.VertexShader(meshVS)
			.PixelShader(meshPS);

		InputElementDesc inputDesc[] =
		{
			{"POSITION", 0, RenderFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PER_VERTEX, 0 },
			{"COLOR", 0, RenderFormat::R32G32B32_FLOAT, 1, 0, InputClassification::PER_VERTEX, 0 },
		};

		meshPSO = CreateGraphicsPipelineState(psoDesc, inputDesc, ARRAYSIZE(inputDesc));
	}

	float rot = 0.0f;

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

		Render_BeginFrame();

		struct
		{
			matrix viewProjection;
		} viewConsts;

		viewConsts.viewProjection = viewMatrix * G.projectionMatrix;

		DynamicBuffer_t viewCbuf = CreateDynamicConstantBuffer(&viewConsts, sizeof(viewConsts));

		struct
		{
			matrix transform;
		} meshConsts;

		meshConsts.transform = MakeMatrixRotationAxis(float3(1, 1, 1), rot);

		DynamicBuffer_t meshCbuf = CreateDynamicConstantBuffer(&meshConsts, sizeof(meshConsts));

		rot += 0.01f;

		Render_BeginRenderFrame();

		CommandListSubmissionGroup clGroup(CommandListType::GRAPHICS);

		CommandList* cl = clGroup.CreateCommandList();

		UploadBuffers(cl);

		cl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::PRESENT, ResourceTransitionState::RENDER_TARGET);

		cl->SetRootSignature();

		// Bind and clear targets
		{
			RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();

			constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.2f, 0.0f };

			cl->ClearRenderTarget(backBufferRtv, DefaultClearCol);
			cl->ClearDepth(G.depthDsv, 1.0f);

			cl->SetRenderTargets(&backBufferRtv, 1, G.depthDsv);
		}

		// Init viewport and scissor
		{
			Viewport vp{ G.screenWidth, G.screenHeight};
			cl->SetViewports(&vp, 1);
			cl->SetDefaultScissor();
		}

		// Draw mesh
		{
			cl->SetPipelineState(meshPSO);

			if (Render_BindlessMode())
			{
				cl->SetGraphicsRootCBV(0, viewCbuf);
				cl->SetGraphicsRootCBV(1, meshCbuf);
			}
			else
			{
				cl->BindVertexCBVs(0, 1, &viewCbuf);
				cl->BindVertexCBVs(1, 1, &meshCbuf);
			}

			cl->SetVertexBuffers(0, 1, &mesh.positionBuf.buf, &mesh.positionBuf.stride, &mesh.positionBuf.offset);
			cl->SetVertexBuffers(1, 1, &mesh.colorBuf.buf, &mesh.colorBuf.stride, &mesh.colorBuf.offset);
			cl->SetIndexBuffer(mesh.indexBuf.buf, mesh.indexBuf.format, mesh.indexBuf.offset);
			cl->DrawIndexedInstanced(mesh.indexBuf.count, 1, 0, 0, 0);
		}

		cl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::RENDER_TARGET, ResourceTransitionState::PRESENT);

		clGroup.Submit();

		Render_EndFrame();

		view->Present(true);
	}

	Render_ShutDown();

	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RenderView* rv = GetRenderViewForHwnd((intptr_t)hWnd);

	switch (msg)
	{
	//case WM_MOVE:
	//{
	//	RECT r;
	//	GetWindowRect(hWnd, &r);
	//	const int x = (int)(r.left);
	//	const int y = (int)(r.top);
	//	break;
	//}
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			const int w = (int)LOWORD(lParam);
			const int h = (int)HIWORD(lParam);

			if (rv)	rv->Resize(w, h);
			ResizeScreen(w, h);
			return 0;
		}

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
