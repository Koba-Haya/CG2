#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include <string>

class EnemyShootComponent : public AbsoluteEngine::IComponent {
public:
    EnemyShootComponent() = default;
    ~EnemyShootComponent() override = default;

    void Update(float deltaTime) override;

    std::string GetTypeName() const override { return "EnemyShootComponent"; }

    // 発射間隔の設定
    void SetShootInterval(float interval) { shootInterval_ = interval; }
    float GetShootInterval() const { return shootInterval_; }

    // ゲームシーン側から発射タイミングを確認（ポーリング）するための機能
    bool WantToShoot() const { return wantToShoot_; }
    void ClearShootFlag() { wantToShoot_ = false; }

private:
    float shootInterval_ = 2.0f; // デフォルトは2秒に1回
    float shootTimer_ = 0.0f;
    bool wantToShoot_ = false;
};
