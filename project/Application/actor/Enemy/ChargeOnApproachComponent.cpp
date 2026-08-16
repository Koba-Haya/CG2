#include "ChargeOnApproachComponent.h"
#include "AbsoluteEngine/scene/BaseScene.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "../../actor/Player/PlayerComponent.h"
#include "GameCamera.h"
#include <cmath>

void ChargeOnApproachComponent::Update(float deltaTime) {
    if (!owner_) return;

    auto scene = BaseScene::GetActiveScene();
    if (!scene) return;
    auto* camera = scene->GetMainCamera();
    if (!camera) return;

    auto& t = owner_->GetTransform();

    if (triggered_) {
        // トリガー時に凍結したカメラ相対オフセットを、現在のカメラ姿勢で再構成する。
        // （PlayerComponent::Updateの worldPos = eye + forward*dist + right*x + up*y と同じ式）
        const Vector3 eye     = camera->GetEye();
        const Vector3 forward = camera->GetForward();
        const Vector3 right   = camera->GetRight();
        const Vector3 up      = camera->GetUp();

        const Vector3 targetPos = {
            eye.x + forward.x * frozenDist_ + right.x * frozenLocalX_ + up.x * frozenLocalY_,
            eye.y + forward.y * frozenDist_ + right.y * frozenLocalX_ + up.y * frozenLocalY_,
            eye.z + forward.z * frozenDist_ + right.z * frozenLocalX_ + up.z * frozenLocalY_
        };

        Vector3 toTarget = {
            targetPos.x - t.translate.x,
            targetPos.y - t.translate.y,
            targetPos.z - t.translate.z
        };
        const float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        if (dist > 0.001f) {
            const Vector3 dir = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };
            t.translate.x += dir.x * chargeSpeed_ * deltaTime;
            t.translate.y += dir.y * chargeSpeed_ * deltaTime;
            t.translate.z += dir.z * chargeSpeed_ * deltaTime;
        }
        return;
    }

    // --- 未トリガー時: プレイヤーとの距離を監視するのみ（自身の移動は行わない） ---
    std::shared_ptr<AbsoluteEngine::GameObject> playerObj = nullptr;
    for (const auto& obj : scene->GetRootObjects()) {
        if (obj && obj->GetComponent<PlayerComponent>()) {
            playerObj = obj;
            break;
        }
    }
    if (!playerObj) return;

    const Vector3 currentPos = t.translate;
    const Vector3 playerPos = playerObj->GetTransform().translate;
    Vector3 toPlayer = {
        playerPos.x - currentPos.x,
        playerPos.y - currentPos.y,
        playerPos.z - currentPos.z
    };
    const float dist = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
    if (dist > triggerRange_) return;

    // トリガー発火: プレイヤーの現在位置を「カメラ相対オフセット」に分解して凍結する
    triggered_ = true;

    const Vector3 eye     = camera->GetEye();
    const Vector3 forward = camera->GetForward();
    const Vector3 right   = camera->GetRight();
    const Vector3 up      = camera->GetUp();

    const Vector3 rel = {
        playerPos.x - eye.x,
        playerPos.y - eye.y,
        playerPos.z - eye.z
    };
    frozenDist_    = rel.x * forward.x + rel.y * forward.y + rel.z * forward.z;
    frozenLocalX_  = rel.x * right.x   + rel.y * right.y   + rel.z * right.z;
    frozenLocalY_  = rel.x * up.x      + rel.y * up.y      + rel.z * up.z;
}
