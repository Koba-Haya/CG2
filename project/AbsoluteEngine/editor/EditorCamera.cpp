#include "EditorCamera.h"
#include <numbers>
#include <algorithm>

namespace AbsoluteEngine {

void EditorCamera::Initialize() {
    view_ = MakeIdentity4x4();
}

void EditorCamera::Update(const Input& input) {
    // ======================
    // 平行移動
    // ======================
    const float keyMoveSpeed = 0.05f;
    const float mousePanSpeed = 0.02f;
    const float wheelMoveSpeed = 0.002f;

    Vector3 move = { 0.0f, 0.0f, 0.0f };

    // キーボード移動（ローカル座標系：右クリック中のみ有効）
    if (input.IsMouseDown(1)) {
        if (input.PressKey(DIK_W)) { move.z += keyMoveSpeed; }
        if (input.PressKey(DIK_S)) { move.z -= keyMoveSpeed; }
        if (input.PressKey(DIK_A)) { move.x -= keyMoveSpeed; }
        if (input.PressKey(DIK_D)) { move.x += keyMoveSpeed; }
        if (input.PressKey(DIK_Q)) { move.y += keyMoveSpeed; }
        if (input.PressKey(DIK_E)) { move.y -= keyMoveSpeed; }
    }

    auto mouse = input.GetMouse();

    // 中ボタン押下中：ドラッグでパン（平行移動）
    if (input.IsMouseDown(2)) {
        move.x += -mouse.dx * mousePanSpeed;
        move.y +=  mouse.dy * mousePanSpeed; // エディタカメラはY軸移動も有効化
    }

    // ホイール前後移動（ローカル Z）
    if (mouse.wheel != 0) {
        move.z += mouse.wheel * wheelMoveSpeed;
    }

    // ローカル移動を回転でワールド方向へ
    move = TransformNormal(move, matRot_);
    translate_ = Add(translate_, move);

    // ======================
    // 回転
    // ======================
    const float rotSpeedKey = 0.05f;
    const float rotSpeedMouse = 0.005f;

    Matrix4x4 rotDelta = MakeIdentity4x4();

    // 右ドラッグ：Yaw, Pitch 回転
    if (input.IsMouseDown(1)) {
        yaw_ += mouse.dx * rotSpeedMouse;   // これが横回転
        pitch_ += mouse.dy * rotSpeedMouse; // これが縦回転
    }

    const float limit = (std::numbers::pi_v<float> * 0.5f) - 0.001f;
    pitch_ = std::clamp(pitch_, -limit, +limit);

    Matrix4x4 rotY = MakeRotateYMatrix(yaw_);
    Matrix4x4 rotX = MakeRotateXMatrix(pitch_);
    Matrix4x4 rotZ = MakeRotateZMatrix(roll_);
    matRot_ = Multiply(rotZ, Multiply(rotX, rotY));

    // ======================
    // View 行列更新
    // ======================
    UpdateMatrix();
}

void EditorCamera::UpdateMatrix() {
    Vector3 cameraPosition = TransformNormal(translate_, matRot_);
    cameraPosition.x += shakeOffset_.x;
    cameraPosition.y += shakeOffset_.y;
    cameraPosition.z += shakeOffset_.z;
    Matrix4x4 translateMatrix = MakeTranslateMatrix(cameraPosition);
    Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);
    view_ = Inverse(worldMatrix);
}

void EditorCamera::FocusOn(const Vector3& targetPosition, float distance) {
    // 現在のカメラの向き（matRot_）のZ軸（前方向）ベクトルを取得
    Vector3 forward = { matRot_.m[2][0], matRot_.m[2][1], matRot_.m[2][2] };
    
    // ターゲットから forward 方向に distance だけ手前に引いた位置をカメラ位置にする
    translate_ = { targetPosition.x - forward.x * distance,
                   targetPosition.y - forward.y * distance,
                   targetPosition.z - forward.z * distance };
    
    // View行列の再計算
    UpdateMatrix();
}

} // namespace AbsoluteEngine
