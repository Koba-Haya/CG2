#pragma once
#include "Type/Vector.h"
#include "AbsoluteEngine/scene/Component.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include <string>

class HomingBulletComponent : public AbsoluteEngine::IComponent {
public:
  HomingBulletComponent() = default;
  ~HomingBulletComponent() override = default;

  void Initialize(const Vector3& initialVelocity, AbsoluteEngine::GameObject* target);
  void Update(float deltaTime) override;
  void OnCollision(AbsoluteEngine::GameObject* other) override;
  
  std::string GetTypeName() const override { return "HomingBulletComponent"; }

  bool IsActive() const { return isActive_; }
  void Deactivate() { isActive_ = false; }
  float GetCollisionRadius() const { return radius_; }

private:
  Vector3 velocity_{ 0, 0, 0 };
  AbsoluteEngine::GameObject* target_ = nullptr;
  
  bool isActive_ = false;
  float lifeTimer_ = 0.0f;
  float maxLife_ = 3.0f; // 3秒で消滅
  float radius_ = 0.5f;  // 当たり判定半径
  float speed_ = 45.0f;  // 通常弾より少し速い
  float homingStrength_ = 5.0f; // 旋回性能
};
