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
  ImGui::SetNextWindowPos(ImVec2(640, 260), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::Begin("Title", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);

  ImGui::SetWindowFontScale(3.0f);
  ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "ABSOLUTE STRIKER"); // 仮タイトル名
  ImGui::SetWindowFontScale(1.0f);

  ImGui::Spacing();
  ImGui::SetWindowFontScale(1.5f);
  ImGui::Text("SPACE / ENTER でスタート");
  ImGui::SetWindowFontScale(1.0f);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.6f, 1.0f), "操作方法");
  ImGui::BulletText("WASD / 矢印キー : 移動");
  ImGui::BulletText("Shift            : 回避ロール（発動中は無敵）");
  ImGui::BulletText("SPACE / 左クリック : 長押しでロックオン、離すと発射");

  ImGui::Spacing();
  if (ImGui::Button("Start", ImVec2(160, 40))) {
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
