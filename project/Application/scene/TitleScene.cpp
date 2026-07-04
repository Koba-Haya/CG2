#include "TitleScene.h"
#include "Input.h"
#include "Renderer.h"
#include "Method.h"
#include "AbsoluteEngine/editor/EditorCamera.h"

#include "ImGuiManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include "SceneIds.h"

void TitleScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);
  startRequested_ = false;
  Renderer::GetInstance()->InitializePostProcess(1280, 720);
}

void TitleScene::Finalize() {}

void TitleScene::Update() {
  if (services_.input) {
    if (services_.input->TriggerKey(DIK_SPACE) || services_.input->TriggerKey(DIK_RETURN) || services_.input->WasPadPressed(XINPUT_GAMEPAD_A)) {
      RequestSceneChange(SceneId::Game);
    }
  }

#ifdef USE_IMGUI
  ImGui::SetNextWindowSize(ImVec2(500.0f, 100.0f), ImGuiCond_Always);
  ImGui::Begin("Title");
  if (ImGui::Button("Start (Space or Enter)")) {
    RequestSceneChange(SceneId::Game);
  }
  ImGui::End();
#endif
}

void TitleScene::Draw() {
  auto* renderer = Renderer::GetInstance();
  renderer->BeginRenderScene();

  Matrix4x4 projInverse;
  if (editorCamera_) {
      projInverse = Inverse(editorCamera_->GetProjectionMatrix());
  } else {
      projInverse = MakeIdentity4x4();
  }

  renderer->EndRenderScene(projInverse);
}
