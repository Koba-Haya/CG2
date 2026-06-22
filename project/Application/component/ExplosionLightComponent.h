#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include <algorithm>

class ExplosionLightComponent : public AbsoluteEngine::IComponent {
public:
    void Update(float deltaTime) override {
        if (!owner_) return;

        timer_ += deltaTime;
        if (timer_ >= lifeTime_) {
            isDead_ = true;
            owner_->GetLight().type = AbsoluteEngine::LightComponent::Type::None;
        } else {
            float t = timer_ / lifeTime_;
            float currentIntensity = maxIntensity_ * (1.0f - t);
            owner_->GetLight().intensity = currentIntensity;
        }
    }

    std::string GetTypeName() const override { return "ExplosionLightComponent"; }
    bool IsDead() const { return isDead_; }

    float timer_ = 0.0f;
    float lifeTime_ = 0.5f;
    float maxIntensity_ = 5.0f;
    bool isDead_ = false;
};
