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

    // タグのみで判定する（名前検索は「EnemyBullet」に「Enemy」が含まれるなど誤反応を招くため廃止）
    const std::string& ownerTag = owner_->GetTag();
    const std::string& otherTag = other->GetTag();

    if (ownerTag == "EnemyBullet") {
        // 敵弾 → プレイヤー本体にのみ反応（プレイヤー弾とは相殺しない）
        if (otherTag == "Player") {
            isActive_ = false;
            owner_->Destroy();
        }
    } else if (ownerTag == "PlayerBullet") {
        // プレイヤー弾 → 敵・ボス本体にのみ反応（敵弾とは相殺しない）
        if (otherTag == "Enemy" || otherTag == "Boss") {
            isActive_ = false;
            owner_->Destroy();
        }
    }
}
