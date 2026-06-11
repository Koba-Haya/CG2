#pragma once
#include "Vector.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include <memory>

class Enemy {
public:
  Enemy() = default;
  ~Enemy() = default;
  Enemy(Enemy &&) noexcept = default;
  Enemy &operator=(Enemy &&) noexcept = default;

  void Initialize(const Vector3 &pos, std::shared_ptr<ModelResource> modelRes);
  void Update(float deltaTime);
  void Draw();


  bool IsActive() const { return isActive_; }
  void OnHit(); // 弾が当たった時の処理

  const Vector3 &GetPosition() const { return position_; }
  float GetCollisionRadius() const { return radius_; }

private:
  Vector3 position_{ 0, 0, 0 };
  Vector3 rotation_{ 0, 0, 0 };
  ModelInstance modelInstance_;
  
  bool isActive_ = false;
  float radius_ = 2.0f; // 少し大きめの当たり判定
};
