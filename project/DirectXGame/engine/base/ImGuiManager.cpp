#include "ImGuiManager.h"

#ifdef USE_IMGUI

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

#include <cassert>

void ImGuiManager::Initialize(WinApp *winApp, DirectXCommon *dxCommon) {
  assert(winApp);
  assert(dxCommon);

  winApp_ = winApp;
  dxCommon_ = dxCommon;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  // Win32 側初期化（資料通り WinApp 依存）
  ImGui_ImplWin32_Init(winApp_->GetHwnd());

  // ImGui が使う SRV を 1つ確保（SrvAllocator の 0 は予約、1以降を使う想定）
  SrvAllocator &srvAlloc = dxCommon_->GetSrvAllocator();
  imguiSrvIndex_ = srvAlloc.Allocate();

  // DX12 側初期化
  // ※ backbuffer 数はあなたのエンジンでは 2 固定のはずなので 2
  // ※ RTV format は DirectXCommon 側が SRGB を使ってる
  ImGui_ImplDX12_Init(dxCommon_->GetDevice(), 2,
                      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, dxCommon_->GetSRVHeap(),
                      srvAlloc.Cpu(imguiSrvIndex_),
                      srvAlloc.Gpu(imguiSrvIndex_));

  // 日本語フォント等（必要なら）
  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontDefault();
}

void ImGuiManager::Finalize() {
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  dxCommon_ = nullptr;
  imguiSrvIndex_ = 0;
}

void ImGuiManager::Begin() {
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void ImGuiManager::End() { ImGui::Render(); }

void ImGuiManager::Draw() {
  // 描画はコマンドリストに積むだけ
  ID3D12GraphicsCommandList *cmd = dxCommon_->GetCommandList();
  // ディスクリプターヒープの配列をセット
  ID3D12DescriptorHeap *heaps[] = {dxCommon_->GetSRVHeap()};
  cmd->SetDescriptorHeaps(_countof(heaps), heaps);
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
}

#endif
