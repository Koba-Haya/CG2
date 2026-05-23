#include "DebugCamera.h"
#include <numbers>
#include <algorithm>

void DebugCamera::Initialize() {
    // projection_ は GameApp 側の camera_->SetPerspective(...) で設定する想定
    // ここでは view_ の初期化だけしておく
    view_ = MakeIdentity4x4();
}

void DebugCamera::Update(const Input& input) {
    // ======================
    // 回転
    // ======================
    const float rotSpeedKey = 0.05f;
    const float rotSpeedMouse = 0.005f;

    auto mouse = input.GetMouse();

    // キーボード回転
    if (input.PressKey(DIK_LEFT))  { yaw_   -= rotSpeedKey; }
    if (input.PressKey(DIK_RIGHT)) { yaw_   += rotSpeedKey; }
    if (input.PressKey(DIK_UP))    { pitch_ -= rotSpeedKey; }
    if (input.PressKey(DIK_DOWN))  { pitch_ += rotSpeedKey; }
    if (input.PressKey(DIK_Z))     { roll_  += rotSpeedKey; }
    if (input.PressKey(DIK_C))     { roll_  -= rotSpeedKey; }

    // 右ドラッグ：Yaw, Pitch 回転
    if (input.IsMouseDown(1)) {
      yaw_ += mouse.dx * rotSpeedMouse;   // 横回転
      pitch_ += mouse.dy * rotSpeedMouse; // 縦回転
    }

    const float limit = (std::numbers::pi_v<float> * 0.5f) - 0.001f;
    pitch_ = std::clamp(pitch_, -limit, +limit);

    Matrix4x4 rotY = MakeRotateYMatrix(yaw_);
    Matrix4x4 rotX = MakeRotateXMatrix(pitch_);
    Matrix4x4 rotZ = MakeRotateZMatrix(roll_);
    matRot_ = Multiply(rotZ, Multiply(rotX, rotY));

    // ======================
    // 平行移動
    // ======================
    const float keyMoveSpeed = 0.5f;
    const float mousePanSpeed = 0.02f;
    const float wheelMoveSpeed = 0.005f;

    Vector3 localMove = { 0.0f, 0.0f, 0.0f };

    // キーボード移動（ローカル座標系）
    if (input.PressKey(DIK_W)) { localMove.z += keyMoveSpeed; }
    if (input.PressKey(DIK_S)) { localMove.z -= keyMoveSpeed; }
    if (input.PressKey(DIK_A)) { localMove.x -= keyMoveSpeed; }
    if (input.PressKey(DIK_D)) { localMove.x += keyMoveSpeed; }
    if (input.PressKey(DIK_Q)) { localMove.y += keyMoveSpeed; }
    if (input.PressKey(DIK_E)) { localMove.y -= keyMoveSpeed; }

    // 中ボタン押下中：ドラッグでパン（平行移動）
    if (input.IsMouseDown(2)) {
        localMove.x -= mouse.dx * mousePanSpeed;
        localMove.y += mouse.dy * mousePanSpeed;
    }

    // ホイール前後移動（ローカル Z）
    if (mouse.wheel != 0) {
        localMove.z += mouse.wheel * wheelMoveSpeed;
    }

    // ローカル移動を現在の回転でワールド移動に変換して加算
    Vector3 worldMove = TransformNormal(localMove, matRot_);
    translate_ = Add(translate_, worldMove);

    // ======================
    // View 行列更新
    // ======================
    Matrix4x4 translateMatrix = MakeTranslateMatrix(translate_);
    // ワールド行列 = R * T (※ DirectXTKなどの左乗算系の場合。このエンジンのMultiplyの仕様に合わせて順序を組む)
    // 既存コードに倣い、matRot_ -> translateMatrix の順で掛けることで、移動成分がそのまま translate_ になるようにする
    Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);

    view_ = Inverse(worldMatrix);
}
