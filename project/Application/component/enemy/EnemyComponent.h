#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include <string>

class EnemyComponent : public AbsoluteEngine::IComponent {
public:
    EnemyComponent() = default;
    ~EnemyComponent() override = default;

    void Update(float deltaTime) override;

    std::string GetTypeName() const override { return "EnemyComponent"; }

    bool IsActive() const { return isActive_; }
    void OnHit(); // 弾が当たった時の処理

    float GetCollisionRadius() const { return radius_; }

private:
    bool isActive_ = true;
    float radius_ = 2.0f; // 少し大きめの当たり判定
};
