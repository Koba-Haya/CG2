#pragma once
#include "BaseScene.h"
#include "Camera.h"
#include <memory>

class GameScene final : public BaseScene {
public:
  GameScene() = default;
  ~GameScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw() override;

private:
  std::unique_ptr<Camera> camera_;
  // ここにゲーム本編のメンバを追加していく
};