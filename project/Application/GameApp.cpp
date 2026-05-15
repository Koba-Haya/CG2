#define NOMINMAX
#include "GameApp.h"

#include "SceneFactory.h"
#include "SceneIds.h"
#include "SceneManager.h"
#include "Renderer.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif

GameApp::GameApp() = default;
GameApp::~GameApp() = default;

void GameApp::Initialize() {
  sceneManager_ = std::make_unique<SceneManager>();

  SceneServices services{};
  services.input = &GetInput();
  services.audio = &GetAudio();
#ifdef USE_IMGUI
  services.imgui = &GetImGui();
#endif
  services.framework = this;

  Renderer::GetInstance()->Initialize(&GetDX());
  sceneManager_->Initialize(services);
  sceneManager_->SetFactory(std::make_unique<SceneFactory>());
  sceneManager_->Start(SceneId::Title);
}

void GameApp::Finalize() {
  if (sceneManager_) {
    sceneManager_->Finalize();
    sceneManager_.reset();
  }
}

void GameApp::Update() {
  if (!sceneManager_) {
    return;
  }

  #ifdef USE_IMGUI
  // デバッグメニュー
  ImGui::Begin("DebugMenu");
  if (ImGui::Button("Go to DevScene")) {
    sceneManager_->RequestChange(SceneId::Dev);
  }
  ImGui::End();
#endif

  sceneManager_->Update();
  sceneManager_->ApplySceneChangeIfNeeded();
}

void GameApp::Draw() {
  auto *cmdList = GetCmdList();

  if (sceneManager_) {
    sceneManager_->Draw();
  }
}
