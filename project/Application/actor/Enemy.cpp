#include "Enemy.h"
#include "Method.h"

void Enemy::Initialize(const Vector3 &pos, std::shared_ptr<ModelResource> modelRes) {
  position_ = pos;
  rotation_ = { 0.0f, 0.0f, 0.0f };
  modelInstance_.Initialize({ modelRes, {1, 1, 1, 1}, 0 });
  isActive_ = true;
}

void Enemy::Update(float deltaTime) {
  if (!isActive_) return;

  // 少し回転させて生きている感を出す
  rotation_.y += 1.0f * deltaTime;
  rotation_.z += 0.5f * deltaTime;

  Matrix4x4 mat = MakeAffineMatrix(Vector3{1.5f, 1.5f, 1.5f}, rotation_, position_);
  modelInstance_.SetWorld(mat);
}

void Enemy::Draw() {
  if (!isActive_) return;
  modelInstance_.Draw();
}

void Enemy::OnHit() {
  isActive_ = false;
}
