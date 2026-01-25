#include "SceneManager.h"

#include <cassert>

#include "BaseScene.h"
#include "ISceneFactory.h"

SceneManager::~SceneManager() = default;

void SceneManager::Initialize(const SceneServices &services) {
  services_ = services;
}

void SceneManager::SetFactory(std::unique_ptr<ISceneFactory> factory) {
  factory_ = std::move(factory);
}

void SceneManager::Start(const std::string &firstSceneId) {
  SetNextSceneById_(firstSceneId);
  ApplySceneChangeIfNeeded();
}

void SceneManager::RequestChange(const std::string &nextSceneId) {
  hasPendingChange_ = true;
  pendingSceneId_ = nextSceneId;
}

void SceneManager::SetNextSceneById_(const std::string &sceneId) {
  assert(factory_ && "SceneManager: factory is null. Call SetFactory().");
  next_ = factory_->Create(sceneId, services_);
  assert(next_ && "SceneManager: factory returned null. Unknown sceneId?");
}

void SceneManager::ApplySceneChangeIfNeeded() {
  if (hasPendingChange_) {
    SetNextSceneById_(pendingSceneId_);
    hasPendingChange_ = false;
    pendingSceneId_.clear();
  }

  if (!next_) {
    return;
  }

  if (current_) {
    current_->Finalize();
    current_.reset();
  }

  current_ = std::move(next_);
  current_->SetSceneManager(this);
  current_->Initialize(services_);
}

void SceneManager::Update() {
  if (current_) {
    current_->Update();
  }
}

void SceneManager::Draw(ID3D12GraphicsCommandList *cmdList) {
  if (current_) {
    current_->Draw(cmdList);
  }
}

void SceneManager::Finalize() {
  if (current_) {
    current_->Finalize();
    current_.reset();
  }
  if (next_) {
    next_->Finalize();
    next_.reset();
  }
  factory_.reset();
}
