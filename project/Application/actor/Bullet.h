#pragma once
#include "Vector.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include <memory>

class Bullet {
public:
  Bullet() = default;
  ~Bullet() = default;
  Bullet(Bullet &&) noexcept = default;
  Bullet &operator=(Bullet &&) noexcept = default;

  void Initialize(const Vector3 &pos, const Vector3 &vel, std::shared_ptr<ModelResource> modelRes);
  void Update(float deltaTime);
  void Draw();


  bool IsActive() const { return isActive_; }
  void Deactivate() { isActive_ = false; }

  const Vector3 &GetPosition() const { return position_; }
  float GetCollisionRadius() const { return radius_; }

private:
  Vector3 position_{ 0, 0, 0 };
  Vector3 velocity_{ 0, 0, 0 };
  ModelInstance modelInstance_;
  
  bool isActive_ = false;
  float lifeTimer_ = 0.0f;
  float maxLife_ = 3.0f; // 3秒で消滅 (constを解除してムーブ代入可能に)
  float radius_ = 0.5f;  // 当たり判定半径 (constを解除)
};
