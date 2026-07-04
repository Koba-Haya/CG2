#include "HomingBulletComponent.h"
#include "../Enemy/EnemyComponent.h"
#include "../Enemy/BossComponent.h"
#include <cmath>

void HomingBulletComponent::Initialize(const Vector3& initialVelocity, AbsoluteEngine::GameObject* target) {
    velocity_ = initialVelocity;
    target_ = target;
    isActive_ = true;
    lifeTimer_ = 0.0f;
}

void HomingBulletComponent::Update(float deltaTime) {
    if (!owner_ || !isActive_) return;

    lifeTimer_ += deltaTime;
    if (lifeTimer_ >= maxLife_) {
        owner_->Destroy();
        return;
    }

    // ターゲットが有効かチェック（死んでいる場合は追尾をやめて直進する）
    bool hasTarget = false;
    if (target_) {
        // nullになっていないか、死んでいないかチェック
        // エンジンによってはDestroy()後にポインタが無効になるが、
        // ここではEnemyComponent等の死亡フラグで判定する
        auto enemyComp = target_->GetComponent<EnemyComponent>();
        auto bossComp = target_->GetComponent<BossComponent>();
        if (enemyComp && !enemyComp->IsDead()) {
            hasTarget = true;
        } else if (bossComp && bossComp->GetHp() > 0) {
            hasTarget = true;
        } else {
            target_ = nullptr; // ターゲット喪失
        }
    }

    auto& t = owner_->GetTransform();

    if (hasTarget) {
        Vector3 targetPos = target_->GetTransform().translate;
        Vector3 currentPos = t.translate;

        // ターゲットへの方向ベクトル
        Vector3 toTarget = {
            targetPos.x - currentPos.x,
            targetPos.y - currentPos.y,
            targetPos.z - currentPos.z
        };
        
        // 正規化
        float len = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        if (len > 0.001f) {
            toTarget.x /= len;
            toTarget.y /= len;
            toTarget.z /= len;
        }

        // 現在の速度ベクトルの正規化
        float vLen = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
        Vector3 currentDir = {0, 0, 1}; // デフォルト
        if (vLen > 0.001f) {
            currentDir.x = velocity_.x / vLen;
            currentDir.y = velocity_.y / vLen;
            currentDir.z = velocity_.z / vLen;
        }

        // 補間（Lerpで徐々にターゲット方向へ向ける）
        float tLerp = homingStrength_ * deltaTime;
        if (tLerp > 1.0f) tLerp = 1.0f;

        Vector3 newDir = {
            currentDir.x + (toTarget.x - currentDir.x) * tLerp,
            currentDir.y + (toTarget.y - currentDir.y) * tLerp,
            currentDir.z + (toTarget.z - currentDir.z) * tLerp
        };

        // 新しい方向を正規化
        float nLen = std::sqrt(newDir.x * newDir.x + newDir.y * newDir.y + newDir.z * newDir.z);
        if (nLen > 0.001f) {
            newDir.x /= nLen;
            newDir.y /= nLen;
            newDir.z /= nLen;
        }

        // 速度の更新
        velocity_.x = newDir.x * speed_;
        velocity_.y = newDir.y * speed_;
        velocity_.z = newDir.z * speed_;
    }

    // 座標の更新
    t.translate.x += velocity_.x * deltaTime;
    t.translate.y += velocity_.y * deltaTime;
    t.translate.z += velocity_.z * deltaTime;

    // 進行方向に向ける回転
    float vxzLen = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
    float yaw = std::atan2(velocity_.x, velocity_.z);
    float pitch = std::atan2(-velocity_.y, vxzLen);
    t.rotate = { pitch, yaw, 0.0f };
}

void HomingBulletComponent::OnCollision(AbsoluteEngine::GameObject* other) {
    if (!isActive_) return;
    
    // 敵に当たったら消滅
    if (other->GetTag() == "Enemy" || other->GetTag() == "Boss" || 
        other->GetName().find("Enemy") != std::string::npos ||
        other->GetName().find("Boss") != std::string::npos) {
        isActive_ = false;
        owner_->Destroy();
    }
}
