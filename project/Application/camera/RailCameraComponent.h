#pragma once
#include "../../AbsoluteEngine/scene/Component.h"
#include "../../AbsoluteEngine/Type/Matrix.h"
#include "../../AbsoluteEngine/Type/Vector.h"
#include <vector>
#include <string>

class GameCamera;
class RailCameraComponent : public AbsoluteEngine::IComponent {
public:
    RailCameraComponent();
    ~RailCameraComponent() override = default;

    void Update(float deltaTime) override;

    std::string GetTypeName() const override { return "RailCameraComponent"; }

    void Serialize(nlohmann::json& j) const override;
    void Deserialize(const nlohmann::json& j) override;

    // レールのウェイポイントを設定・取得
    void SetWaypoints(const std::vector<Vector3>& points) { waypoints_ = points; }
    const std::vector<Vector3>& GetWaypoints() const { return waypoints_; }
    std::vector<Vector3>& GetWaypointsRef() { return waypoints_; }
    
    // ウェイポイントの編集機能
    void AddWaypoint(const Vector3& pos);
    void InsertWaypoint(size_t index, const Vector3& pos);
    void RemoveWaypoint(size_t index);

    // エディタUI描画
    void DrawInspectorUI() override;
    void DrawGizmo(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, float windowPosX, float windowPosY, float windowSizeX, float windowSizeY);
    void HandleMousePicking(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, float windowPosX, float windowPosY, float windowSizeX, float windowSizeY);

    // パラメータ調整用
    void SetSpeed(float speed) { speed_ = speed; }
    void SetLookAhead(float offset) { lookAheadOffset_ = offset; }
    void ResetProgress() { progress_ = 0.0f; }
    float GetProgress() const { return progress_; }
    // タイムラインからカメラ進行度を直接設定するセッター（シーク同期用）
    // progress_ を書き換えるだけでなく、即座にカメラ位置を再計算して反映する
    // これによりエディットモードでのスライダー操作が即座に画面に反映される
    void SetProgress(float progress);

    // 変更検知用（エディタ連携など）
    bool ConsumeModifiedFlag() {
        bool flag = isModified_;
        isModified_ = false;
        return flag;
    }
    void SetModifiedFlag() { isModified_ = true; }

private:
    // カメラ位置の計算とSet処理を共通化したヘルパー（UpdateとSetProgressから呼ばれる）
    void ApplyCameraTransform_();

    std::vector<Vector3> waypoints_;
    float progress_ = 0.0f;
    float speed_ = 0.01f;
    float lookAheadOffset_ = 0.01f;

    // エディタ操作用
    int selectedPointIndex_ = -1;
    bool isGizmoUsing_ = false;
    bool isModified_ = false;

    std::vector<Vector3> waypointsBeforeEdit_;
};
