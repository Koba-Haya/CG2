#include "EnemyShootComponent.h"

void EnemyShootComponent::Update(float deltaTime) {
    // 弾の発射要求がゲームシーン側で処理されるまではタイマーを進めない
    if (wantToShoot_) {
        return;
    }

    shootTimer_ += deltaTime;
    if (shootTimer_ >= shootInterval_) {
        wantToShoot_ = true;
        shootTimer_ -= shootInterval_; // 残りの時間を引き継ぐ
    }
}
