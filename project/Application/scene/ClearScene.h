#pragma once
#include "BaseScene.h"

class ClearScene final : public BaseScene {
public:
  ClearScene() = default;
  ~ClearScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw() override;
};
