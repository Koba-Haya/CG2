#include "TitleScene.h"

#include "ImGuiManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "SceneIds.h"

void TitleScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);
  startRequested_ = false;
}

void TitleScene::Finalize() {}

void TitleScene::Update() {
#ifdef USE_IMGUI
  services_.imgui->Begin();

  ImGui::Begin("Title");
  if (ImGui::Button("Start")) {
    RequestSceneChange(SceneId::Game);
  }
  ImGui::End();

  services_.imgui->End();
#endif
}

void TitleScene::Draw(ID3D12GraphicsCommandList *cmdList) { (void)cmdList; }
