#pragma once
#include "ICameraController.h"
#include "Spline.h"
#include <vector>

class RailCameraController : public ICameraController {
public:
    RailCameraController();
    ~RailCameraController() override = default;

    void Update(GameCamera& camera, const CameraContext& ctx) override;

    // レールのウェイポイントを設定・取得
    void SetWaypoints(const std::vector<Vector3>& points) { waypoints_ = points; }
    const std::vector<Vector3>& GetWaypoints() const { return waypoints_; }
    
    // パラメータ調整用
    void SetSpeed(float speed) { speed_ = speed; }
    void SetLookAhead(float offset) { lookAheadOffset_ = offset; }

    void ResetProgress() { progress_ = 0.0f; }
    float GetProgress() const { return progress_; }

private:
    std::vector<Vector3> waypoints_;
    float progress_ = 0.0f;
    float speed_ = 0.01f;
    float lookAheadOffset_ = 0.01f;
};
