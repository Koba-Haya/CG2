#pragma once
#include "ICameraController.h"
#include "Spline.h"
#include <vector>
#include "../../AbsoluteEngine/Type/Matrix.h"
#include "../../AbsoluteEngine/Type/Vector.h"

namespace AbsoluteEngine {
    class EditorCamera;
}

class RailCameraController : public ICameraController {
public:
    RailCameraController();
    ~RailCameraController() override = default;

    void Update(GameCamera& camera, const CameraContext& ctx) override;

    // レールのウェイポイントを設定・取得
    void SetWaypoints(const std::vector<Vector3>& points) { waypoints_ = points; }
    const std::vector<Vector3>& GetWaypoints() const { return waypoints_; }
    std::vector<Vector3>& GetWaypointsRef() { return waypoints_; }
    
    // ウェイポイントの編集機能
    void AddWaypoint(const Vector3& pos);
    void InsertWaypoint(size_t index, const Vector3& pos);
    void RemoveWaypoint(size_t index);

    // エディタUI描画
    void DrawEditorUI(const Vector3& cameraPos = Vector3(0,0,0));
    void DrawGizmo(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, float windowPosX, float windowPosY, float windowSizeX, float windowSizeY);
    void HandleMousePicking(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, float windowPosX, float windowPosY, float windowSizeX, float windowSizeY);

    // パラメータ調整用
    void SetSpeed(float speed) { speed_ = speed; }

    // 変更検知用
    bool ConsumeModifiedFlag() {
        bool flag = isModified_;
        isModified_ = false;
        return flag;
    }

    void SetLookAhead(float offset) { lookAheadOffset_ = offset; }

    void ResetProgress() { progress_ = 0.0f; }
    float GetProgress() const { return progress_; }

private:
    std::vector<Vector3> waypoints_;
    float progress_ = 0.0f;
    float speed_ = 0.01f;
    float lookAheadOffset_ = 0.01f;

    // エディタ操作用
    int selectedPointIndex_ = -1;
    bool isGizmoUsing_ = false;
    bool isModified_ = false;
};
