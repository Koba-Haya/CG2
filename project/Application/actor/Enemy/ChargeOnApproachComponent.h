#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "Type/Vector.h"
#include <string>

/// <summary>
/// ホーミング接近型敵の突進コンポーネント。
/// トリガー前は何もしない（StraightMoveComponent等、別の移動コンポーネントに
/// 動きを任せる想定）。プレイヤーとの距離がtriggerRange_以下になった瞬間、
/// その時点のプレイヤー位置への方向を一度だけ確定し、以後はその直線方向へ
/// 突進し続ける（以後は追尾しない）。
/// </summary>
class ChargeOnApproachComponent : public AbsoluteEngine::IComponent {
public:
    ChargeOnApproachComponent() = default;
    ~ChargeOnApproachComponent() override = default;

    void Update(float deltaTime) override;

    std::string GetTypeName() const override { return "ChargeOnApproachComponent"; }

private:
    float triggerRange_ = 15.0f; // この距離以下でプレイヤーへの突進が発火する
    float chargeSpeed_  = 25.0f; // 突進中の移動速度

    bool triggered_ = false;

    // トリガー発火時にプレイヤーの位置を「カメラ相対オフセット」に分解して凍結したもの。
    // プレイヤーのワールド座標は eye + forward*dist + right*x + up*y で毎フレーム
    // 再計算される（カメラがレールを前進し続けるため、操作しなくても値は変わり続ける）。
    // dist/x/y だけを凍結し、eye/forward/right/up は毎フレーム最新のカメラ姿勢を使うことで、
    // 「プレイヤーの操作による回避には追従しないが、カメラ自身のレール前進には追従する」
    // 狙いを再現する。
    float frozenDist_ = 0.0f;
    float frozenLocalX_ = 0.0f;
    float frozenLocalY_ = 0.0f;
};
