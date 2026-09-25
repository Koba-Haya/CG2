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

protected:
  // ゲームオーバー画面はレール演出を持たないためタイムライン機能は不要
  bool UsesTimeline() const override { return false; }
};
