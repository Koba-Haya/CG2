#include "DebugCamera.h"

void DebugCamera::Initialize() {
    // projection_ は GameApp 側の camera_->SetPerspective(...) で設定する想定
    // ここでは view_ の初期化だけしておく
    view_ = MakeIdentity4x4();
}

void DebugCamera::Update(const Input& input) {
    // ======================
    // 平行移動
    // ======================
    const float keyMoveSpeed = 0.05f;
    const float mousePanSpeed = 0.02f;
    const float wheelMoveSpeed = 0.002f;

    Vector3 move = { 0.0f, 0.0f, 0.0f };

    // キーボード移動（ローカル座標系）
    if (input.IsDown(DIK_W)) { move.z += keyMoveSpeed; }
    if (input.IsDown(DIK_S)) { move.z -= keyMoveSpeed; }
    if (input.IsDown(DIK_A)) { move.x -= keyMoveSpeed; }
    if (input.IsDown(DIK_D)) { move.x += keyMoveSpeed; }
    if (input.IsDown(DIK_Q)) { move.y += keyMoveSpeed; }
    if (input.IsDown(DIK_E)) { move.y -= keyMoveSpeed; }

    // マウス状態
    auto mouse = input.GetMouse();

    // 中ボタン押下中：ドラッグでパン（平行移動）
    if (input.IsMouseDown(2)) {
        move.x += -mouse.dx * mousePanSpeed;
        // move.y +=  mouse.dy * mousePanSpeed; // 必要なら有効化
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

    // キーボード回転
    if (input.IsDown(DIK_LEFT)) { rotDelta = Multiply(MakeRotateYMatrix(+rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_RIGHT)) { rotDelta = Multiply(MakeRotateYMatrix(-rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_UP)) { rotDelta = Multiply(MakeRotateXMatrix(+rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_DOWN)) { rotDelta = Multiply(MakeRotateXMatrix(-rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_Z)) { rotDelta = Multiply(MakeRotateZMatrix(+rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_C)) { rotDelta = Multiply(MakeRotateZMatrix(-rotSpeedKey), rotDelta); }

    // 右ドラッグ：Yaw 回転
    if (input.IsMouseDown(1)) {
        float yaw = -mouse.dx * rotSpeedMouse;
        rotDelta = Multiply(MakeRotateYMatrix(yaw), rotDelta);
    }

    matRot_ = Multiply(rotDelta, matRot_);

    // ======================
    // View 行列更新
    // ======================
    // ※あなたの現行ロジックを保持（挙動が変わると困るので）
    Vector3 cameraPosition = TransformNormal(translate_, matRot_);
    Matrix4x4 translateMatrix = MakeTranslateMatrix(cameraPosition);
    Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);

    view_ = Inverse(worldMatrix);
}
