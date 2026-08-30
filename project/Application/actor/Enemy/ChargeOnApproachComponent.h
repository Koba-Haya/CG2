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
    // 発火時に一度だけ確定する突進方向。以後は毎フレーム再計算しない
    // （再計算すると、目標付近で行き過ぎては戻る振動が発生するため）。
    Vector3 chargeDir_ = { 0, 0, 0 };

    // カメラの移動速度を推定するための前フレームeye座標。
    // プレイヤーのワールド座標はカメラのeye基準で毎フレーム再計算されるため、
    // 操作していなくてもカメラのレール前進と同じ速度で流れ続ける。
    // 発火の瞬間、この推定速度ぶんだけ「到達する頃の位置」を先読みして狙う。
    Vector3 prevEye_ = { 0, 0, 0 };
    bool hasPrevEye_ = false;
};
