#include "DebugCamera.h"

void DebugCamera::Initialize() {}

void DebugCamera::Update(const Input& input) {
    // ======================
    // 平行移動
    // ======================
    const float keyMoveSpeed = 0.05f;  // WASDQE 用
    const float mousePanSpeed = 0.02f;  // 左ドラッグ平行移動の感度
    const float wheelMoveSpeed = 0.002f; // ホイール前進/後退の感度

    Vector3 move = { 0.0f, 0.0f, 0.0f };

    // --- キーボード移動（ローカル座標系）---
    if (input.IsDown(DIK_W)) { move.z += keyMoveSpeed; }
    if (input.IsDown(DIK_S)) { move.z -= keyMoveSpeed; }
    if (input.IsDown(DIK_A)) { move.x -= keyMoveSpeed; }
    if (input.IsDown(DIK_D)) { move.x += keyMoveSpeed; }
    if (input.IsDown(DIK_Q)) { move.y += keyMoveSpeed; }
    if (input.IsDown(DIK_E)) { move.y -= keyMoveSpeed; }

    // --- マウス状態 ---
    auto mouse = input.GetMouse(); // ← これで MouseState エラーは出なくなる

    // 左クリック中はドラッグで左右平行移動（ドラッグ方向の逆にカメラを動かす）
    if (input.IsMouseDown(2)) { // 2 = 中ボタン
        // 左にドラッグ(dx<0) → カメラを右(+X)へ動かすように符号を反転
        move.x += -mouse.dx * mousePanSpeed;
        // 必要なら縦方向もつけるなら↓を有効にする
        // move.y +=  mouse.dy * mousePanSpeed;
    }

    // ホイールで前進・後退（ローカル Z 方向）
    if (mouse.wheel != 0) {
        // 上スクロール（wheel>0）で前進、下スクロールで後退
        move.z += mouse.wheel * wheelMoveSpeed;
    }

    // ローカル→ワールドの向きに変換して位置へ足す
    move = TransformNormal(move, matRot_);
    translate_ = Add(translate_, move);

    // ======================
    // 回転
    // ======================
    const float rotSpeedKey = 0.05f;   // 矢印キー / ZC の回転速度
    const float rotSpeedMouse = 0.005f;  // 右ドラッグ回転の感度

    Matrix4x4 rotDelta = MakeIdentity4x4();

    // --- キーボード回転 ---
    if (input.IsDown(DIK_LEFT)) { rotDelta = Multiply(MakeRotateYMatrix(+rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_RIGHT)) { rotDelta = Multiply(MakeRotateYMatrix(-rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_UP)) { rotDelta = Multiply(MakeRotateXMatrix(+rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_DOWN)) { rotDelta = Multiply(MakeRotateXMatrix(-rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_Z)) { rotDelta = Multiply(MakeRotateZMatrix(+rotSpeedKey), rotDelta); }
    if (input.IsDown(DIK_C)) { rotDelta = Multiply(MakeRotateZMatrix(-rotSpeedKey), rotDelta); }

    // --- 右ドラッグで左右回転 ---
    if (input.IsMouseDown(1)) { // 1 = 右ボタン
        // マウスを左に動かす(dx<0)と「右回転」になるように符号を反転
        float yaw = -mouse.dx * rotSpeedMouse;
        Matrix4x4 yawMat = MakeRotateYMatrix(yaw);
        rotDelta = Multiply(yawMat, rotDelta);
    }

    // 回転反映
    matRot_ = Multiply(rotDelta, matRot_);

    // ======================
    // ビュー行列更新
    // ======================
    Vector3 cameraPosition = TransformNormal(translate_, matRot_);
    Matrix4x4 translateMatrix = MakeTranslateMatrix(cameraPosition);
    Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);
    viewMatrix_ = Inverse(worldMatrix);
}
