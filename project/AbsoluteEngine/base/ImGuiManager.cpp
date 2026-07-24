#include "ImGuiManager.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "SrvAllocator.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#endif

#include <Windows.h>
#include <cassert>
#include <string>
#include <filesystem>

#ifdef USE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);
#endif

void ImGuiManager::Initialize(WinApp* winApp, DirectXCommon* dx) {
	assert(winApp != nullptr);
	assert(dx != nullptr);

	if (initialized_) {
		return; // 二重初期化防止（"Already initialized..." 対策）
	}

	winApp_ = winApp;
	dx_ = dx;

#ifdef USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// --- Docking を有効化 ---
	// ViewportsEnable はOSウィンドウへのポップアウト機能だが、
	// 別スワップチェーン作成が必要で環境依存のクラッシュが起きるため無効化する
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // ウィンドウ同士のドッキング

	// --- 日本語フォントの読み込み ---
	// ImGui標準フォントはCJKグリフを持たないため、日本語UIテキストが豆腐（文字化け）表示になる。
	// 追加アセットを同梱せず、Windows同梱の日本語フォントをOSインストール先パスから直接読み込む。
	// これらのフォントはASCIIグリフも含むため、デフォルトフォントとの併用は不要。
	// 候補が全て無い/読み込み失敗の場合はImGui標準フォントにフォールバックする。
	{
		char windowsDir[MAX_PATH] = {};
		UINT dirLen = GetWindowsDirectoryA(windowsDir, MAX_PATH);
		ImFont* loadedFont = nullptr;

		if (dirLen > 0 && dirLen < MAX_PATH) {
			static const char* kJapaneseFontCandidates[] = {
				"\\Fonts\\meiryo.ttc",
				"\\Fonts\\YuGothM.ttc",
				"\\Fonts\\msgothic.ttc",
			};

			for (const char* candidate : kJapaneseFontCandidates) {
				std::string fontPath = std::string(windowsDir) + candidate;
				if (!std::filesystem::exists(fontPath)) {
					continue;
				}
				ImFontConfig fontConfig{};
				loadedFont = io.Fonts->AddFontFromFileTTF(
					fontPath.c_str(), 18.0f, &fontConfig, io.Fonts->GetGlyphRangesJapanese());
				if (loadedFont) {
					break;
				}
			}
		}

		if (!loadedFont) {
			io.Fonts->AddFontDefault();
		}
	}

	ImGui::StyleColorsDark();

	// Win32 backend 初期化
	ImGui_ImplWin32_Init(winApp_->GetHwnd());

	// DX12 backend 初期化（新API: ImGui_ImplDX12_InitInfo 構造体を使用）
	ImGui_ImplDX12_InitInfo dx12Info{};
	dx12Info.Device            = dx_->GetDevice();
	dx12Info.CommandQueue      = dx_->GetCommandQueue();     // テクスチャアップロード用
	dx12Info.NumFramesInFlight = dx_->GetBackBufferCount();
	dx12Info.RTVFormat         = dx_->GetRTVFormat();
	dx12Info.DSVFormat         = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dx12Info.SrvDescriptorHeap = dx_->GetSRVHeap();

	// SRV ディスクリプタの確保コールバック（SrvAllocator 経由で動的に割り当て）
	dx12Info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu,
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
	{
		auto* dx = static_cast<DirectXCommon*>(info->UserData);
		SrvAllocator& alloc = dx->GetSrvAllocator();
		uint32_t index = alloc.Allocate();
		*out_cpu = alloc.Cpu(index);
		*out_gpu = alloc.Gpu(index);
	};

	// SRV ディスクリプタの解放コールバック（現状 SrvAllocator に Free がなければ何もしない）
	dx12Info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* /*info*/,
		D3D12_CPU_DESCRIPTOR_HANDLE /*cpu*/,
		D3D12_GPU_DESCRIPTOR_HANDLE /*gpu*/)
	{
		// 将来 SrvAllocator に Free 機能を追加した場合はここで呼ぶ
	};

	// UserData に DirectXCommon を渡してコールバック内で参照できるようにする
	dx12Info.UserData = dx_;

	ImGui_ImplDX12_Init(&dx12Info);

	// WinApp に「メッセージフック」を登録（WinApp は ImGui を知らない）
	winApp_->SetMessageHandler([](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> bool {
		if (ImGui::GetCurrentContext() == nullptr) {
			return false;
		}
		return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp) != 0;
	});

	initialized_ = true;
#else
	initialized_ = false;
#endif
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
	if (!initialized_) {
		return;
	}

	if (frameBegun_) {
		End();
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
#else
	winApp_ = nullptr;
	dx_ = nullptr;
#endif
}

void ImGuiManager::Begin() {
#ifdef USE_IMGUI
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
#endif
}

void ImGuiManager::End() {
#ifdef USE_IMGUI
	if (!initialized_) {
		return;
	}
	if (!frameBegun_) {
		return;
	}

	ImGui::EndFrame();
	ImGui::Render();

	frameBegun_ = false;
#endif
}

void ImGuiManager::Draw(ID3D12GraphicsCommandList* cmdList) {
#ifdef USE_IMGUI
	if (!initialized_) {
		return;
	}
	assert(cmdList != nullptr);

	if (ImGui::GetCurrentContext() == nullptr) {
		return;
	}
	ImDrawData* drawData = ImGui::GetDrawData();
	if (drawData == nullptr) {
		return;
	}

	// EndFrame() が呼ばれてないと "g.WithinFrameScope" 系で落ちるので保険
	if (frameBegun_) {
		End();
	}

	ID3D12DescriptorHeap* heaps[] = { dx_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
#endif
}

void ImGuiManager::UpdateViewports() {
#ifdef USE_IMGUI
	if (!initialized_) {
		return;
	}

	// ViewportsEnable 時：ImGui がポップアウトした OS ウィンドウを更新・描画する
	// PostDraw（Present）の後に呼ぶ必要がある
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
	}
#endif
}
