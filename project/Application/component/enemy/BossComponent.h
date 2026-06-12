#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include <string>

class BossComponent : public AbsoluteEngine::IComponent {
public:
    BossComponent() = default;
    ~BossComponent() override = default;

    void Update(float deltaTime) override;

    std::string GetTypeName() const override { return "BossComponent"; }

    bool IsActive() const { return isActive_; }
    void TakeDamage(int damage);

    float GetCollisionRadius() const { return radius_; }

    int GetHp() const { return hp_; }
    int GetMaxHp() const { return maxHp_; }

private:
    bool isActive_ = true;
    float radius_ = 5.0f; // ボスなので当たり判定を大きめに
    int hp_ = 20;         // ボスのHP
    int maxHp_ = 20;
};
