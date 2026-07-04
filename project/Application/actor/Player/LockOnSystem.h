#pragma once
#include <vector>
#include <memory>
#include "Type/Vector.h"

// 前方宣言
namespace AbsoluteEngine {
    class GameObject;
}
class BaseScene;
class Input;
class GameCamera;

class LockOnSystem {
public:
    LockOnSystem() = default;
    ~LockOnSystem() = default;

    void Initialize();
    
    // 長押し時間のカウントや敵の検索を行う
    void Update(float deltaTime, Input* input, GameCamera* camera, BaseScene* scene, const Vector2& cursorPos);

    // 長押しを離したときにホーミング弾を発射する
    void FireHomingBullets(BaseScene* scene, const Vector3& spawnPos);

    bool IsLockingMode() const { return isLockingMode_; }
    const std::vector<AbsoluteEngine::GameObject*>& GetLockedTargets() const { return lockedTargets_; }
    
    // HUDに渡すための、ロック中対象のスクリーン座標を計算して返す
    std::vector<Vector2> GetLockedScreenPositions(GameCamera* camera) const;

private:
    std::vector<AbsoluteEngine::GameObject*> lockedTargets_;
    int maxLockTargets_ = 8; // パンツァードラグーン式（最大8体）

    float holdTimer_ = 0.0f;
    float lockOnThreshold_ = 0.2f; // 何秒長押ししたらロックオンモードに入るか
    bool isLockingMode_ = false;

    // 敵をスクリーン上に収めているか判定し、ロックする処理
    void SearchAndLockTargets(GameCamera* camera, BaseScene* scene, const Vector2& cursorPos);
};
