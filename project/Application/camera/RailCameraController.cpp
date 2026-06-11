#define NOMINMAX
#include "RailCameraController.h"
#include "GameCamera.h"
#include "Method.h"
#include <algorithm>

RailCameraController::RailCameraController() {
    waypoints_ = {
        { 0.0f,  5.0f, -50.0f}, // P0: スタート（少し手前から）
        { 0.0f,  3.0f,   0.0f}, // P1: 序盤の直進エリア
        { 20.0f, 8.0f,  70.0f}, // P2: 大きく右へ上昇カーブ
        {-15.0f, 2.0f, 140.0f}, // P3: 左下へと急降下旋回
        { 0.0f,  5.0f, 220.0f}  // P4: ゴールへと向かう直進路
    };
    speed_ = 0.10f; // 進行感を実感できるようスピードを向上
    lookAheadOffset_ = 0.02f;
}

void RailCameraController::Update(GameCamera& camera, const CameraContext& ctx) {
    if (waypoints_.size() < 2) return;

    // 進捗の更新
    progress_ += speed_ * ctx.deltaTime;
    if (progress_ > 1.0f) progress_ = 1.0f;

    // 現在地点の計算
    Vector3 currentPos = Spline::GetPoint(waypoints_, progress_);

    // 注視点の計算 (Look-ahead)
    float targetT = std::min(progress_ + lookAheadOffset_, 1.0f);
    Vector3 targetPos = Spline::GetPoint(waypoints_, targetT);

    // カメラの設定
    camera.SetEye(currentPos);
    camera.SetTarget(targetPos);
    camera.SetUp({ 0.0f, 1.0f, 0.0f });
}
