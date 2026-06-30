#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include <string>

class StraightMoveComponent : public AbsoluteEngine::IComponent {
public:
    StraightMoveComponent() = default;
    ~StraightMoveComponent() override = default;

    void Update(float deltaTime) override;

    std::string GetTypeName() const override { return "StraightMoveComponent"; }

    // 移動速度の設定・取得
    void SetSpeed(float speed) { speed_ = speed; }
    float GetSpeed() const { return speed_; }

private:
    float speed_ = 10.0f; // デフォルトの移動速度
};
