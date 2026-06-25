#define NOMINMAX
#include "RailCameraController.h"
#include "GameCamera.h"
#include "Method.h"
#include <algorithm>
#include <string>

#ifdef USE_IMGUI
#include <imgui.h>
#include "../../externals/ImGuizmo/ImGuizmo.h"
#endif

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
    Vector3 targetPos;
    if (progress_ >= 1.0f - 0.001f) {
        // 終点付近では、少し手前の点から終点への方向を向くようにする
        Vector3 p1 = Spline::GetPoint(waypoints_, 1.0f - lookAheadOffset_);
        Vector3 p2 = Spline::GetPoint(waypoints_, 1.0f);
        Vector3 diff = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
        targetPos = { currentPos.x + diff.x, currentPos.y + diff.y, currentPos.z + diff.z };
    } else {
        float targetT = std::min(progress_ + lookAheadOffset_, 1.0f);
        targetPos = Spline::GetPoint(waypoints_, targetT);
    }

    // カメラの設定
    camera.SetEye(currentPos);
    camera.SetTarget(targetPos);
    camera.SetUp({ 0.0f, 1.0f, 0.0f });
}

void RailCameraController::AddWaypoint(const Vector3& pos) {
    waypoints_.push_back(pos);
    isModified_ = true;
}

void RailCameraController::InsertWaypoint(size_t index, const Vector3& pos) {
    if (index <= waypoints_.size()) {
        waypoints_.insert(waypoints_.begin() + index, pos);
        isModified_ = true;
    }
}

void RailCameraController::RemoveWaypoint(size_t index) {
    if (index < waypoints_.size() && waypoints_.size() > 3) {
        waypoints_.erase(waypoints_.begin() + index);
        if (selectedPointIndex_ >= static_cast<int>(waypoints_.size())) {
            selectedPointIndex_ = static_cast<int>(waypoints_.size()) - 1;
        }
        isModified_ = true;
    }
}

void RailCameraController::DrawEditorUI(const Vector3& cameraPos) {
#ifdef USE_IMGUI
    ImGui::Begin("Rail Camera Editor");
    ImGui::Text("Waypoints: %d", static_cast<int>(waypoints_.size()));

    if (ImGui::Button("Add Point")) {
        Vector3 newPos = waypoints_.empty() ? Vector3(0,0,0) : waypoints_.back();
        newPos.x += 10.0f;
        AddWaypoint(newPos);
        selectedPointIndex_ = static_cast<int>(waypoints_.size()) - 1;
    }

    ImGui::Separator();
    ImGui::Text("Points List (Right-Click for menu):");

    for (int i = 0; i < static_cast<int>(waypoints_.size()); ++i) {
        std::string label = "Point " + std::to_string(i);
        if (ImGui::Selectable(label.c_str(), selectedPointIndex_ == i)) {
            selectedPointIndex_ = i;
        }

        // ポイント単位の右クリックメニュー
        if (ImGui::BeginPopupContextItem(("PointContextMenu" + std::to_string(i)).c_str(), ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::MenuItem("Insert After")) {
                Vector3 newPos = waypoints_[i];
                newPos.z += 5.0f; // 適当なオフセット
                InsertWaypoint(i + 1, newPos);
            }
            if (ImGui::MenuItem("Delete")) {
                RemoveWaypoint(i);
            }
            ImGui::EndPopup();
        }
    }

    ImGui::Separator();
    if (selectedPointIndex_ >= 0 && selectedPointIndex_ < static_cast<int>(waypoints_.size())) {
        ImGui::Separator();
        ImGui::Text("Selected Point: %d", selectedPointIndex_);
        
        float p[3] = { waypoints_[selectedPointIndex_].x, waypoints_[selectedPointIndex_].y, waypoints_[selectedPointIndex_].z };
        if (ImGui::DragFloat3("Position", p, 0.1f)) {
            waypoints_[selectedPointIndex_] = { p[0], p[1], p[2] };
            isModified_ = true;
        }
    }

    ImGui::End();
#endif
}

void RailCameraController::DrawGizmo(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, float windowPosX, float windowPosY, float windowSizeX, float windowSizeY) {
#ifdef USE_IMGUI
    if (selectedPointIndex_ >= 0 && selectedPointIndex_ < static_cast<int>(waypoints_.size())) {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(windowPosX, windowPosY, windowSizeX, windowSizeY);

        // Transform行列の作成 (位置のみ)
        Matrix4x4 pointTransform = MakeTranslateMatrix(waypoints_[selectedPointIndex_]);
        float objectMatrix[16];
        memcpy(objectMatrix, &pointTransform.m[0][0], sizeof(float) * 16);

        bool wasGizmoUsing = isGizmoUsing_;
        isGizmoUsing_ = ImGuizmo::Manipulate(
            &viewMatrix.m[0][0], 
            &projectionMatrix.m[0][0],
            ImGuizmo::TRANSLATE, 
            ImGuizmo::WORLD, 
            objectMatrix
        );

        if (isGizmoUsing_) {
            float matrixTranslation[3], matrixRotation[3], matrixScale[3];
            ImGuizmo::DecomposeMatrixToComponents(objectMatrix, matrixTranslation, matrixRotation, matrixScale);
            
            Vector3 newPos = { matrixTranslation[0], matrixTranslation[1], matrixTranslation[2] };
            if (waypoints_[selectedPointIndex_].x != newPos.x || 
                waypoints_[selectedPointIndex_].y != newPos.y || 
                waypoints_[selectedPointIndex_].z != newPos.z) {
                waypoints_[selectedPointIndex_] = newPos;
            }
        }

        if (wasGizmoUsing && !isGizmoUsing_) {
            isModified_ = true;
        }
    }
#endif
}

void RailCameraController::HandleMousePicking(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, float windowPosX, float windowPosY, float windowSizeX, float windowSizeY) {
#ifdef USE_IMGUI
    if (!ImGui::IsWindowHovered() || ImGuizmo::IsOver() || isGizmoUsing_) return;

    if (ImGui::IsMouseClicked(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        
        float u = (mousePos.x - windowPosX) / windowSizeX;
        float v = (mousePos.y - windowPosY) / windowSizeY;
        if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return;

        float ndcX = u * 2.0f - 1.0f;
        float ndcY = 1.0f - v * 2.0f;

        Matrix4x4 viewProj = Multiply(viewMatrix, projectionMatrix);
        Matrix4x4 invViewProj = Inverse(viewProj);

        Vector3 nearPos = TransformPoint({ ndcX, ndcY, 0.0f }, invViewProj);
        Vector3 farPos = TransformPoint({ ndcX, ndcY, 1.0f }, invViewProj);
        Vector3 rayDir = Normalize({ farPos.x - nearPos.x, farPos.y - nearPos.y, farPos.z - nearPos.z });

        float closestDist = -1.0f;
        int hitIndex = -1;
        float pointRadius = 2.0f; // ピッキング用の判定半径

        for (int i = 0; i < static_cast<int>(waypoints_.size()); ++i) {
            Vector3 m = { nearPos.x - waypoints_[i].x, nearPos.y - waypoints_[i].y, nearPos.z - waypoints_[i].z };
            float b = Dot(m, rayDir);
            float c = Dot(m, m) - pointRadius * pointRadius;
            
            if (c > 0.0f && b > 0.0f) continue;
            
            float discriminant = b * b - c;
            if (discriminant >= 0.0f) {
                float t = -b - std::sqrt(discriminant);
                if (t < 0.0f) t = 0.0f;
                if (closestDist < 0.0f || t < closestDist) {
                    closestDist = t;
                    hitIndex = i;
                }
            }
        }

        if (hitIndex != -1) {
            selectedPointIndex_ = hitIndex;
        }
    }
#endif
}
