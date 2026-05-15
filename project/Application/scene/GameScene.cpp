#define NOMINMAX
#include "GameScene.h"
#include "Renderer.h"
#include "DebugCamera.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif

void GameScene::Initialize(const SceneServices &services) {
  BaseScene::Initialize(services);

  camera_ = std::make_unique<DebugCamera>();
  camera_->Initialize();
  camera_->SetPerspective(0.45f, Renderer::GetInstance()->GetAspectRatio(), 0.1f, 1000.0f);
}

void GameScene::Finalize() {
}

void GameScene::Update() {
  if (camera_) {
    camera_->Update(*services_.input);
  }
  
#ifdef USE_IMGUI
  ImGui::Begin("GameScene");
  ImGui::End();
#endif
}

void GameScene::Draw() {
  auto* renderer = Renderer::GetInstance();
  if (camera_) {
    renderer->SetCamera(*camera_);
  }

  // グリッドなどの描画
  renderer->RenderPrimitives();
}