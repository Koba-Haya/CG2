#include "EnemyComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "AbsoluteEngine/scene/DissolveComponent.h"
#include <algorithm>

void EnemyComponent::Update(float deltaTime) {
    if (!owner_) return;

    if (isDead_) {
        dissolveTimer_ += deltaTime;
        float t = std::clamp(dissolveTimer_ / dissolveDuration_, 0.0f, 1.0f);
        
        if (auto dissolveComp = owner_->GetComponent<AbsoluteEngine::DissolveComponent>()) {
            auto& dissolve = *dissolveComp;
            dissolve.enable = true;
            dissolve.threshold = t;
            dissolve.edgeRange = 0.05f;
            dissolve.edgeColor = {1.0f, 0.0f, 0.0f}; // 赤色のエッジ
            dissolve.maskColor = {1.0f, 0.0f, 0.0f}; 
            
            if (t >= 1.0f) {
                owner_->Destroy();
            }
        } else {
            // DissolveComponentがない場合は即時消滅
            owner_->Destroy();
        }
    }
}

void EnemyComponent::OnHit() {
    if (!isDead_) {
        isDead_ = true;
        isActive_ = false;
        dissolveTimer_ = 0.0f;
    }
}

void EnemyComponent::OnCollision(AbsoluteEngine::GameObject* other) {
    if (!isActive_ || isDead_) return;
    
    // 自機弾に当たったらダメージ
    if (other->GetName().find("PlayerBullet") != std::string::npos || other->GetTag() == "PlayerBullet") {
        OnHit();
    }
}
