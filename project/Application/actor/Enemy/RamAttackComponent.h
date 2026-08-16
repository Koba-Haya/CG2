#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include <string>

/// <summary>
/// 体当たり攻撃コンポーネント。プレイヤーと衝突した瞬間に自爆撃破する。
/// プレイヤー側へのダメージ処理はPlayerComponent::OnCollisionが
/// タグ判定("Enemy")で既に行っているため、ここでは自身の撃破のみを担当する。
/// </summary>
class RamAttackComponent : public AbsoluteEngine::IComponent {
public:
    RamAttackComponent() = default;
    ~RamAttackComponent() override = default;

    void OnCollision(AbsoluteEngine::GameObject* other) override;

    std::string GetTypeName() const override { return "RamAttackComponent"; }
};
