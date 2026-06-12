#pragma once
#include "BaseScene.h"

class GameOverScene final : public BaseScene {
public:
  GameOverScene() = default;
  ~GameOverScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw() override;
};
