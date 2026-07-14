#include "EnemyComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "AbsoluteEngine/scene/DissolveComponent.h"
#include <algorithm>

void EnemyComponent::Update(float deltaTime) {
    if (!owner_) return;

    if (isDead_) {
        // -----------------------------------------------------------------------
        // 撃破直後の1回だけ onDestroyed コールバックを発火する。
        // Update は毎フレーム呼ばれるため、effectFired_ フラグで1回に制限する。
        // -----------------------------------------------------------------------
        if (!effectFired_ && onDestroyed) {
            effectFired_ = true;
            onDestroyed(owner_->GetTransform().translate);
        }

        // ディゾルブアニメーションを進める
        dissolveTimer_ += deltaTime;
        float t = std::clamp(dissolveTimer_ / dissolveDuration_, 0.0f, 1.0f);

        if (auto dissolveComp = owner_->GetComponent<AbsoluteEngine::DissolveComponent>()) {
            auto& dissolve = *dissolveComp;
            dissolve.enable = true;
            dissolve.threshold = t;
            dissolve.edgeRange = 0.05f;
            dissolve.edgeColor = { 1.0f, 0.3f, 0.0f }; // オレンジのエッジ
            dissolve.maskColor  = { 1.0f, 0.1f, 0.0f };

            if (t >= 1.0f) {
                owner_->Destroy();
            }
        } else {
            // DissolveComponent がない場合は即時消滅
            owner_->Destroy();
        }
    }
}

void EnemyComponent::OnHit() {
    if (!isDead_) {
        isDead_    = true;
        isActive_  = false;
        dissolveTimer_ = 0.0f;
        // コールバックは Update 内で 1 回だけ発火する（タイミング保証のため）
    }
}

void EnemyComponent::OnCollision(AbsoluteEngine::GameObject* other) {
    if (!isActive_ || isDead_) return;

    // タグ判定：自機弾が当たったらダメージを受ける
    if (other->GetTag() == "PlayerBullet") {
        OnHit();
    }
}
