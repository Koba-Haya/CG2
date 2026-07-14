#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "Type/Vector.h"
#include <functional>
#include <string>

/// <summary>
/// 敵の基本コンポーネント。
/// 死亡・ヒット時にコールバック（onDestroyed）を発火することで、
/// GameScene 等の上位層と疎結合なイベント通知（Observer）を実現する。
/// </summary>
class EnemyComponent : public AbsoluteEngine::IComponent {
public:
    EnemyComponent() = default;
    ~EnemyComponent() override = default;

    void Update(float deltaTime) override;

    std::string GetTypeName() const override { return "EnemyComponent"; }

    bool IsActive() const { return isActive_; }
    void OnCollision(AbsoluteEngine::GameObject* other) override;
    void OnHit(); // 弾が当たった時の処理

    bool IsDead() const { return isDead_; }

    float GetCollisionRadius() const { return radius_; }

    // -----------------------------------------------------------------------
    // イベントコールバック（Observer）
    // -----------------------------------------------------------------------
    // 撃破された瞬間に1回だけ呼ばれる。
    // 引数は爆発座標（= 敵のワールド座標）。
    // GameScene 側で Spawn 時にラムダを登録しておくことで疎結合に演出を発火できる。
    std::function<void(const Vector3&)> onDestroyed;

private:
    bool isActive_ = true;
    bool isDead_ = false;
    bool effectFired_ = false; // onDestroyed コールバックを既に発火済みか
    float radius_ = 2.0f;     // コライダー半径

    float dissolveTimer_ = 0.0f;
    float dissolveDuration_ = 1.0f; // 1秒かけてディゾルブ消滅
};
