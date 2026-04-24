#define NOMINMAX
#include "GameApp.h"

#include "SceneFactory.h"
#include "SceneIds.h"
#include "SceneManager.h"

GameApp::GameApp() = default;
GameApp::~GameApp() = default;

void GameApp::Initialize() {
  sceneManager_ = std::make_unique<SceneManager>();

  SceneServices services{};
  services.dx = &GetDX();
  services.input = &GetInput();
  services.audio = &GetAudio();
  services.imgui = &GetImGui();
  services.framework = this;

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
  sceneManager_->Update();
  sceneManager_->ApplySceneChangeIfNeeded();
}

void GameApp::Draw() {
  auto *cmdList = GetCmdList();

  if (sceneManager_) {
    sceneManager_->Draw(cmdList);
  }
}
