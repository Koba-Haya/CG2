#pragma once
#include "Type/Vector.h"
#include "AbsoluteEngine/scene/Component.h"
#include "AbsoluteEngine/scene/GameObject.h"

class BulletComponent : public AbsoluteEngine::IComponent {
public:
  BulletComponent() = default;
  ~BulletComponent() override = default;

  void Initialize(const Vector3 &vel);
  void Update(float deltaTime) override;
  void OnCollision(AbsoluteEngine::GameObject* other) override;
  
  std::string GetTypeName() const override { return "BulletComponent"; }

  bool IsActive() const { return isActive_; }
  void Deactivate() { isActive_ = false; }
  float GetCollisionRadius() const { return radius_; }

private:
  Vector3 velocity_{ 0, 0, 0 };
  bool isActive_ = false;
  float lifeTimer_ = 0.0f;
  float maxLife_ = 3.0f; // 3秒で消滅
  float radius_ = 0.5f;  // 当たり判定半径
};
