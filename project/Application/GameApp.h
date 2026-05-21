#pragma once
#include "FrameWork.h"
#include <memory>

class SceneManager;

class GameApp final : public AbsoluteFrameWork {
public:
  GameApp();
  ~GameApp() override;

protected:
  void Initialize() override;
  void Finalize() override;
  void Update() override;
  void Draw() override;

private:
  std::unique_ptr<SceneManager> sceneManager_;

  // DockBuilder による初期レイアウト構築済みフラグ
  // true になると以降はフレームごとに再構築しない
  bool dockBuilt_ = false;
};
