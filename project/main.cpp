#include "GameApp.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#include <wrl.h>
#endif

#ifdef _DEBUG
// 終了時にD3D12の生存オブジェクトを列挙する（mainのスコープ最後で走らせる）
struct D3DResourceLeakChecker {
  ~D3DResourceLeakChecker() {
    Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
      debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
      debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
      debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
    }
  }
};
#endif

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#ifdef _DEBUG
  D3DResourceLeakChecker leakChecker;
#endif

  GameApp app;
  return app.Run();
}

// #include <Windows.h>
// #include <d3d12.h>
// #include <dxgi1_6.h>
// #include <string>
//
// class ImGuiManager {
// public:
//   struct InitParams {
//     HWND hwnd = nullptr;
//     ID3D12Device *device = nullptr;
//     int frameCount = 2;
//
//     DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//     ID3D12DescriptorHeap *srvHeap = nullptr;
//
//     // SRVヒープの index=0 を ImGui 用に予約する前提
//     D3D12_CPU_DESCRIPTOR_HANDLE imguiCpuHandle{};
//     D3D12_GPU_DESCRIPTOR_HANDLE imguiGpuHandle{};
//
//     std::string fontPath = "resources/fonts/M_PLUS_1p/MPLUS1p-Regular.ttf";
//     float fontSize = 22.0f;
//   };
//
// public:
//   ImGuiManager() = default;
//   ~ImGuiManager();
//
//   bool Initialize(const InitParams &params);
//   void Shutdown();
//
//   void BeginFrame();
//   void EndFrame();
//
//   void Render(ID3D12GraphicsCommandList *commandList);
//
//   // Win32 WndProc から呼ぶ用（WinApp から ImGui を切り離す）
//   bool HandleWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
//
// private:
//   bool initialized_ = false;
// };

// #include "externals/imgui/imgui.h"
// #include "externals/imgui/imgui_impl_dx12.h"
// #include "externals/imgui/imgui_impl_win32.h"
//
// ImGuiManager::~ImGuiManager() { Shutdown(); }
//
// bool ImGuiManager::Initialize(const InitParams& params) {
//   if (initialized_) {
//     return true;
//   }
//   if (!params.hwnd || !params.device || !params.srvHeap) {
//     return false;
//   }
//
//   IMGUI_CHECKVERSION();
//   ImGui::CreateContext();
//   ImGui::StyleColorsDark();
//
//   if (!ImGui_ImplWin32_Init(params.hwnd)) {
//     return false;
//   }
//
//   if (!ImGui_ImplDX12_Init(params.device,
//                            params.frameCount,
//                            params.rtvFormat,
//                            params.srvHeap,
//                            params.imguiCpuHandle,
//                            params.imguiGpuHandle)) {
//     return false;
//   }
//
//   ImGuiIO& io = ImGui::GetIO();
//   io.Fonts->AddFontFromFileTTF(params.fontPath.c_str(),
//                                params.fontSize,
//                                nullptr,
//                                io.Fonts->GetGlyphRangesJapanese());
//
//   initialized_ = true;
//   return true;
// }
//
// void ImGuiManager::Shutdown() {
//   if (!initialized_) {
//     return;
//   }
//
//   ImGui_ImplDX12_Shutdown();
//   ImGui_ImplWin32_Shutdown();
//
//   if (ImGui::GetCurrentContext()) {
//     ImGui::DestroyContext();
//   }
//
//   initialized_ = false;
// }
//
// bool ImGuiManager::HandleWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
//   if (!initialized_) {
//     return false;
//   }
//   return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp) != 0;
// }
//
// void ImGuiManager::BeginFrame() {
//   if (!initialized_) {
//     return;
//   }
//   ImGui_ImplDX12_NewFrame();
//   ImGui_ImplWin32_NewFrame();
//   ImGui::NewFrame();
// }
//
// void ImGuiManager::EndFrame() {
//   if (!initialized_) {
//     return;
//   }
//   ImGui::Render();
// }
//
// void ImGuiManager::Render(ID3D12GraphicsCommandList* commandList) {
//   if (!initialized_ || !commandList) {
//     return;
//   }
//   ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
// }
