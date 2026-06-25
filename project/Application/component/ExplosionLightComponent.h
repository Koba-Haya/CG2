#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "AbsoluteEngine/scene/LightNodeComponent.h"
#include <algorithm>

class ExplosionLightComponent : public AbsoluteEngine::IComponent {
public:
    void Update(float deltaTime) override {
        if (!owner_) return;

        timer_ += deltaTime;
        auto lightComp = owner_->GetComponent<AbsoluteEngine::LightNodeComponent>();
        if (lightComp) {
            if (timer_ >= lifeTime_) {
                isDead_ = true;
                lightComp->type = AbsoluteEngine::LightNodeComponent::Type::None;
            } else {
                float t = timer_ / lifeTime_;
                float currentIntensity = maxIntensity_ * (1.0f - t);
                lightComp->intensity = currentIntensity;
            }
        } else {
            if (timer_ >= lifeTime_) {
                isDead_ = true;
            }
        }
    }

    std::string GetTypeName() const override { return "ExplosionLightComponent"; }
    bool IsDead() const { return isDead_; }

    float timer_ = 0.0f;
    float lifeTime_ = 0.5f;
    float maxIntensity_ = 5.0f;
    bool isDead_ = false;
};
