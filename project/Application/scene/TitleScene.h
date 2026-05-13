#pragma once
#include "BaseScene.h"
#include <memory>

struct ID3D12GraphicsCommandList;

class Sprite;
class UnifiedPipeline;

class TitleScene final : public BaseScene {
public:
  TitleScene() = default;
  ~TitleScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw() override;

private:
  bool startRequested_ = false;

  // ここは必要なら：タイトル画像スプライトなど
  // とりあえず最低限で「黒背景＋ImGui」でもOK
};
