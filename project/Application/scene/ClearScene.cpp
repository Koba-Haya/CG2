#include "ClearScene.h"
#include "Input.h"
#include "SceneIds.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void ClearScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);
}

void ClearScene::Finalize() {}

void ClearScene::Update() {
  if (services_.input) {
    if (services_.input->TriggerKey(DIK_SPACE) || services_.input->TriggerKey(DIK_RETURN) || services_.input->WasPadPressed(XINPUT_GAMEPAD_A)) {
      RequestSceneChange(SceneId::Title);
    }
    if (services_.input->TriggerKey(DIK_R) || services_.input->WasPadPressed(XINPUT_GAMEPAD_B)) {
      RequestSceneChange(SceneId::Game);
    }
  }

#ifdef USE_IMGUI
  ImGui::SetNextWindowPos(ImVec2(640, 360), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::Begin("Game Clear", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
  
  ImGui::SetWindowFontScale(3.0f);
  ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GAME CLEAR!!");
  ImGui::SetWindowFontScale(1.0f);
  
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::Button("Title", ImVec2(120, 40))) {
    RequestSceneChange(SceneId::Title);
  }
  ImGui::SameLine();
  if (ImGui::Button("Retry", ImVec2(120, 40))) {
    RequestSceneChange(SceneId::Game);
  }

  ImGui::End();
#endif
}

void ClearScene::Draw() {
    // Renderer 等を用いた背景描画を必要に応じて追加
}
