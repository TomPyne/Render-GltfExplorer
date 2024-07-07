#include <Render/Render.h>
#include <SurfMath.h>

#include "backends/imgui_impl_win32.h"
#include "imgui.h"
#include "imgui_impl_render.h"

using namespace tpr;

static struct
{
	uint32_t screenWidth = 0;
	uint32_t screenHeight = 0;
} G;

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
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main()
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "Render Example ImGui", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "Render Example ImGui", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplRender_Init(RenderFormat::R8G8B8A8_UNORM);

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

		// Because we use multiple viewports it is more efficient to sync at the latest possible point
		view->Sync();

		Render_BeginFrame();

		{
			ImGui_ImplRender_NewFrame();

			ImGui_ImplWin32_NewFrame();

			ImGui::NewFrame();

			ImGui::ShowDemoWindow();

			ImGui::Render();
		}

		ImRenderFrameData* frameData = ImGui_ImplRender_PrepareFrameData(ImGui::GetDrawData());

		Render_BeginRenderFrame();

		CommandListSubmissionGroup clGroup(CommandListType::GRAPHICS);

		CommandList* cl = clGroup.CreateCommandList();

		UploadBuffers(cl);

		cl->TransitionResource(view->GetCurrentBackBufferTexture(), ResourceTransitionState::PRESENT, ResourceTransitionState::RENDER_TARGET);

		// Bind and clear targets
		{
			RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();

			constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.2f, 0.0f };

			cl->ClearRenderTarget(backBufferRtv, DefaultClearCol);

			cl->SetRenderTargets(&backBufferRtv, 1, DepthStencilView_t::INVALID);
		}

		{
			cl->SetRootSignature(ImGui_ImplRender_GetRootSignature());

			ImGui_ImplRender_RenderDrawData(frameData, ImGui::GetDrawData(), cl);

			ImGui_ImplRender_ReleaseFrameData(frameData);
		}

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
