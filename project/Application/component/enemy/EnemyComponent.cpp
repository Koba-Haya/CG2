#include "EnemyComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include <algorithm>

void EnemyComponent::Update(float deltaTime) {
    if (!owner_) return;

    if (isDead_) {
        dissolveTimer_ += deltaTime;
        float t = std::clamp(dissolveTimer_ / dissolveDuration_, 0.0f, 1.0f);
        
        auto& dissolve = owner_->GetDissolve();
        dissolve.enable = true;
        dissolve.threshold = t;
    }
}

void EnemyComponent::OnHit() {
    if (!isDead_) {
        isDead_ = true;
        isActive_ = false;
        dissolveTimer_ = 0.0f;
    }
}
