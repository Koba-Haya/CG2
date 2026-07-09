#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "Type/Vector.h"
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

    // 進行方向追跞用（マジックナンバー不要で動的に取得）
    Vector3 prevPos_     = { 0, 0, 0 }; // 前フレームの位置
    Vector3 facingDir_   = { 0, 0, -1 }; // 現在の進行方向（初期値：Z軍手前）
    bool hasPrevPos_     = false; // prevPos_が初期化済みかのフラグ
};
