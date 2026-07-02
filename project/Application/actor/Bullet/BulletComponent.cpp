#include "BulletComponent.h"

#include "AbsoluteEngine/scene/BaseScene.h"
#include "../Enemy/EnemyComponent.h"
#include "../Enemy/BossComponent.h"
#include "Application/actor/Player/PlayerComponent.h"

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
    owner_->Destroy(); // 寿命が来たらオブジェクトごと消滅
    return;
  }

  auto& t = owner_->GetTransform();
  t.translate.x += velocity_.x * deltaTime;
  t.translate.y += velocity_.y * deltaTime;
  t.translate.z += velocity_.z * deltaTime;
}

void BulletComponent::OnCollision(AbsoluteEngine::GameObject* other) {
    if (!isActive_ || !owner_) return;

    bool isEnemyBullet = (owner_->GetName().find("EnemyBullet") != std::string::npos || owner_->GetTag() == "EnemyBullet");
    if (isEnemyBullet) {
        if (other->GetName().find("Player") != std::string::npos || other->GetTag() == "Player") {
            if (other->GetName().find("PlayerBullet") == std::string::npos && other->GetTag() != "PlayerBullet") {
                isActive_ = false;
                owner_->Destroy();
            }
        }
    } else {
        if (other->GetName().find("Enemy") != std::string::npos || other->GetTag() == "Enemy" || 
            other->GetName().find("Boss") != std::string::npos || other->GetTag() == "Boss") {
            if (other->GetName().find("EnemyBullet") == std::string::npos && other->GetTag() != "EnemyBullet") {
                isActive_ = false;
                owner_->Destroy();
            }
        }
    }
}
