#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "Type/Vector.h"
#include "LockOnSystem.h"
#include <memory>

class Input;
class GameCamera;

class PlayerComponent : public AbsoluteEngine::IComponent {
public:
  PlayerComponent() = default;
  ~PlayerComponent() override = default;

  void SetInput(Input* input) { input_ = input; }
  void SetCamera(GameCamera* camera) { camera_ = camera; }

  LockOnSystem lockon_; // 外部からアクセス可能にする

  void Initialize();
  void Update(float deltaTime) override;
  void OnCollision(AbsoluteEngine::GameObject* other) override;

  std::string GetTypeName() const override { return "PlayerComponent"; }

  void TakeDamage(int damage);
  bool IsDead() const;
  int GetHp() const { return hp_; }
  int GetMaxHp() const { return maxHp_; }

  // カーソル（照準）のスクリーン座標を取得
  const Vector2& GetCursorPos() const { return cursorPos_; }

private:
  Input* input_ = nullptr;
  GameCamera* camera_ = nullptr;

  Vector2 cursorPos_{ 640.0f, 360.0f }; // カーソルのスクリーン座標
  float cursorSpeed_ = 600.0f; // カーソルの移動速度（ピクセル/秒）

  Vector2 localPos_{ 0.0f, -0.5f }; // 自機の目標ローカル座標への追従用
  float moveSpeed_ = 5.0f; // 追従速度（Lerpの係数など）
  float cameraDistance_ = 22.0f;

  int hp_ = 5;
  int maxHp_ = 5;
  float shootCooldown_ = 0.0f;
};
