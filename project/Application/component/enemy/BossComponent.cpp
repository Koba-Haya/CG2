#include "BossComponent.h"

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
    }
}
