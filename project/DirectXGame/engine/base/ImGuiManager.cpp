#include "ImGuiManager.h"
#include "WinApp.h"
#include "DirectXCommon.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"

#include <Windows.h>
#include <cassert>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

void ImGuiManager::Initialize(WinApp* winApp, DirectXCommon* dx) {
	assert(winApp != nullptr);
	assert(dx != nullptr);

	if (initialized_) {
		return; // 二重初期化防止（"Already initialized..." 対策）
	}

	winApp_ = winApp;
	dx_ = dx;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// Win32 backend
	ImGui_ImplWin32_Init(winApp_->GetHwnd());

	// DX12 backend
	// DirectXCommon側がこれらを提供できる前提
	ID3D12Device* device = dx_->GetDevice();
	ID3D12DescriptorHeap* srvHeap = dx_->GetSRVHeap();
	const int backBufferCount = dx_->GetBackBufferCount();
	const DXGI_FORMAT rtvFormat = dx_->GetRTVFormat();

	// ImGui用フォントSRVは heap の先頭を使う想定（必要なら allocator で0番固定などにしてね）
	D3D12_CPU_DESCRIPTOR_HANDLE fontCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE fontGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();

	ImGui_ImplDX12_Init(
		device,
		backBufferCount,
		rtvFormat,
		srvHeap,
		fontCpu,
		fontGpu
	);

	// WinAppに「メッセージフック」を登録（WinAppはImGuiを知らない）
	winApp_->SetMessageHandler([](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> bool {
		if (ImGui::GetCurrentContext() == nullptr) {
			return false;
		}
		return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp) != 0;
		});

	initialized_ = true;
}

void ImGuiManager::Finalize() {
	if (!initialized_) {
		return;
	}

	if (frameBegun_) {
		EndFrame();
	}

	// 先にWinAppのフック解除（終了時にWndProcがImGui触らないように）
	if (winApp_) {
		winApp_->SetMessageHandler(nullptr);
	}

	// DX12 -> Win32 -> Context の順でShutdown
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	initialized_ = false;
	frameBegun_ = false;
	winApp_ = nullptr;
	dx_ = nullptr;
}

void ImGuiManager::BeginFrame() {
	if (!initialized_) {
		return;
	}
	if (frameBegun_) {
		return;
	}

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	frameBegun_ = true;
}

void ImGuiManager::EndFrame() {
	if (!initialized_) {
		return;
	}
	if (!frameBegun_) {
		return;
	}

	ImGui::EndFrame();
	ImGui::Render();

	frameBegun_ = false;
}

void ImGuiManager::Draw(ID3D12GraphicsCommandList* cmdList) {
	if (!initialized_) {
		return;
	}
	assert(cmdList != nullptr);

	// EndFrame() が呼ばれてないと "g.WithinFrameScope" 系で落ちるので保険
	if (frameBegun_) {
		EndFrame();
	}

	ID3D12DescriptorHeap* heaps[] = { dx_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}
