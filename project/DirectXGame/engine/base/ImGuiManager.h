#pragma once

// 授業資料の都合で USE_IMGUI を切り替えに使う
// 例：プロジェクトの「プリプロセッサの定義」に USE_IMGUI を追加
// あるいはここで #define USE_IMGUI してもOK（ただし全体に効く）
#ifdef USE_IMGUI

#include "DirectXCommon.h"
#include "WinApp.h"

class ImGuiManager {
public:
  ImGuiManager() = default;
  ~ImGuiManager() = default;

  // WinApp 依存（資料通り）
  void Initialize(WinApp *winApp, DirectXCommon *dxCommon);
  void Finalize();

  void Begin();
  void End();
  void Draw();

private:
  WinApp *winApp_ = nullptr;
  DirectXCommon *dxCommon_ = nullptr;

  // ImGui 用に確保した SRV の index（SrvAllocator の 0番予約ルールに従う）
  UINT imguiSrvIndex_ = 0;
};

#else

// USE_IMGUI 無しでもビルドが通るダミー
class WinApp;
class DirectXCommon;

class ImGuiManager {
public:
  void Initialize(WinApp *, DirectXCommon *) {}
  void Finalize() {}
  void Begin() {}
  void End() {}
  void Draw() {}
};

#endif
