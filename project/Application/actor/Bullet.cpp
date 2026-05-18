#include "Bullet.h"
#include "Method.h"

void Bullet::Initialize(const Vector3 &pos, const Vector3 &vel, std::shared_ptr<ModelResource> modelRes) {
  if (!modelRes) {
      isActive_ = false;
      return;
  }
  position_ = pos;
  velocity_ = vel;
  if (!modelInstance_.Initialize({ modelRes, {1, 1, 1, 1}, 0 })) {
      isActive_ = false;
      return;
  }
  isActive_ = true;
  lifeTimer_ = 0.0f;
}

void Bullet::Update(float deltaTime) {
  if (!isActive_) return;

  lifeTimer_ += deltaTime;
  if (lifeTimer_ >= maxLife_) {
    isActive_ = false;
    return;
  }

  position_.x += velocity_.x * deltaTime;
  position_.y += velocity_.y * deltaTime;
  position_.z += velocity_.z * deltaTime;

  // 弾のモデルサイズを小さめにする
  Matrix4x4 mat = MakeAffineMatrix(Vector3{0.2f, 0.2f, 0.2f}, Vector3{0.0f, 0.0f, 0.0f}, position_);
  modelInstance_.SetWorld(mat);
}

void Bullet::Draw() {
  if (!isActive_) return;
  modelInstance_.Draw();
}
