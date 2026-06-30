#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "Type/Vector.h"
#include <memory>

class Input;
class GameCamera;

class PlayerComponent : public AbsoluteEngine::IComponent {
public:
  PlayerComponent() = default;
  ~PlayerComponent() override = default;

  void SetInput(Input* input) { input_ = input; }
  void SetCamera(GameCamera* camera) { camera_ = camera; }

  void Initialize();
  void Update(float deltaTime) override;
  void OnCollision(AbsoluteEngine::GameObject* other) override;

  std::string GetTypeName() const override { return "PlayerComponent"; }

  void TakeDamage(int damage);
  bool IsDead() const;
  int GetHp() const { return hp_; }
  int GetMaxHp() const { return maxHp_; }

private:
  Input* input_ = nullptr;
  GameCamera* camera_ = nullptr;

  Vector2 localPos_{ 0.0f, -1.0f };
  float moveSpeed_ = 3.0f;
  float cameraDistance_ = 10.0f;

  int hp_ = 5;
  int maxHp_ = 5;
  float shootCooldown_ = 0.0f;
};
