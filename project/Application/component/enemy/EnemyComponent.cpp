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
