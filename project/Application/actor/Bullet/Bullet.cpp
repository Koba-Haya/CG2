#include "Bullet/Bullet.h"

void BulletComponent::Initialize(const Vector3 &vel) {
  velocity_ = vel;
  isActive_ = true;
  lifeTimer_ = 0.0f;
}

void BulletComponent::Update(float deltaTime) {
  if (!isActive_ || !owner_) return;

  lifeTimer_ += deltaTime;
  if (lifeTimer_ >= maxLife_) {
    isActive_ = false;
    return;
  }

  auto& t = owner_->GetTransform();
  t.translate.x += velocity_.x * deltaTime;
  t.translate.y += velocity_.y * deltaTime;
  t.translate.z += velocity_.z * deltaTime;
}
