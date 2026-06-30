#include "BossComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"

void BossComponent::Update(float deltaTime) {
    // Boss固有の更新処理（例: 移動アニメーションなど）をここに追加可能
    (void)deltaTime;
}

void BossComponent::TakeDamage(int damage) {
    if (!isActive_) return;

    hp_ -= damage;
    if (hp_ <= 0) {
        hp_ = 0;
        isActive_ = false; // 撃破
        if (owner_) {
            owner_->Destroy();
        }
    }
}

void BossComponent::OnCollision(AbsoluteEngine::GameObject* other) {
    if (!isActive_) return;
    
    if (other->GetName().find("PlayerBullet") != std::string::npos || other->GetTag() == "PlayerBullet") {
        TakeDamage(1);
    }
}
