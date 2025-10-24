#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <Xinput.h>
#include <Windows.h>
#include <wrl.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xinput9_1_0.lib")

//--------------------------------------------------------------------------------------
// Input クラスの公開インターフェースと状態構造体
//--------------------------------------------------------------------------------------
class Input {
public:
    // namespace省略
    template<class T>using ComPtr=Microsoft::WRL::ComPtr<T>;

public:
  // マウスの瞬間状態（DIMOUSESTATE2 をラップ）
  struct MouseState {
    // 相対移動量（直近 Update 1回分での変化量。+右 / +下 /
    // +ホイール上方向で正）
    LONG dx = 0;    // X 変位
    LONG dy = 0;    // Y 変位
    LONG wheel = 0; // ホイール（lZ の別名）

    // ボタン押下ビット（0: 左 / 1: 右 / 2: 中 / 3-7: サイド等）
    // DIMOUSESTATE2::rgbButtons の各要素が 0x80 なら押下中
    BYTE buttons[8] = {};
  };

  // ゲームパッドの瞬間状態（XINPUT_STATE をラップ）
  struct GamepadState {
    // ボタンビット（XINPUT_GAMEPAD_* を参照）
    WORD buttons = 0;
    // トリガ（0-255）。左/右別々。
    BYTE leftTrigger = 0;
    BYTE rightTrigger = 0;
    // 右手・左手スティックの X/Y 入力（-32768 ～ 32767）
    SHORT lx = 0, ly = 0;
    SHORT rx = 0, ry = 0;
  };

public:
  Input();
  ~Input();

  // 初期化：アプリのインスタンスハンドルとウィンドウハンドルが必要
  // ここで DirectInput と各デバイス（キーボード／マウス）を生成・設定
  // ゲームパッド（XInput）は初期化不要だが、後段の Update で問い合わせる
  bool Initialize(HINSTANCE instanceHandle, HWND windowHandle);

  // 明示終了（デストラクタで自動でも解放される）
  void Finalize();

  // 毎フレーム呼ぶ。キーボード／マウスの Acquire→状態取得、
  // ゲームパッドの XInputGetState を行い、前フレームとの差分を内部に保持
  void Update();

  //======================
  // キーボード（DirectInput）
  //======================
  // 押している間判定（引数は DIK_* を渡す）
  bool IsDown(int dik) const;
  // 今フレームで押された瞬間
  bool WasPressed(int dik) const;
  // 今フレームで離された瞬間
  bool WasReleased(int dik) const;
  // 生の 256 バイト配列
  const BYTE *GetKeyboardState() const;

  //======================
  // マウス（DirectInput / DIMOUSESTATE2）
  //======================
  // 今フレームのマウス相対移動量・ボタン・ホイール
  MouseState GetMouse() const;
  // 指定ボタンが押されているか（0=左,1=右,2=中,...）
  bool IsMouseDown(int buttonIndex) const;
  // 今フレームで押された瞬間
  bool WasMousePressed(int buttonIndex) const;
  // 今フレームで離れた瞬間
  bool WasMouseReleased(int buttonIndex) const;

  //======================
  // ゲームパッド（XInput）
  //======================
  // 接続しているか（0番コントローラ）
  bool IsGamepadConnected() const;
  // 今フレームのゲームパッド瞬間状態
  GamepadState GetGamepad() const;
  // ボタン（XINPUT_GAMEPAD_*）の押している間／瞬間
  bool IsPadDown(WORD xinputButton) const;
  bool WasPadPressed(WORD xinputButton) const;
  bool WasPadReleased(WORD xinputButton) const;

  // デッドゾーンを設定
  void SetThumbDeadZone(short leftDead, short rightDead);

private:
  // コピー禁止
  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;

  // 内部ユーティリティ：DirectInput デバイス喪失時の再取得
  void TryAcquireKeyboard_();
  void TryAcquireMouse_();

  // 内部ユーティリティ：XInput から読み出し、デッドゾーンを適用
  void ReadGamepad_();

private:
  // --- DirectInput 本体 ---
  ComPtr<IDirectInput8> directInput_ = nullptr;

  // --- キーボードデバイス ---
  ComPtr<IDirectInputDevice8> keyboard_ = nullptr;
  BYTE currentKeys_[256] = {};  // 今フレームのキーオン状態
  BYTE previousKeys_[256] = {}; // 1フレーム前のキーオン状態

  // --- マウスデバイス（DIMOUSESTATE2 を使用） ---
  ComPtr<IDirectInputDevice8> mouse_ = nullptr;
  DIMOUSESTATE2 currentMouse_ = {};  // 今フレーム
  DIMOUSESTATE2 previousMouse_ = {}; // 1フレーム前

  // --- ゲームパッド（XInput, 0番パッドを対象）---
  bool gamepadConnected_ = false;
  XINPUT_STATE currentPad_ = {};
  XINPUT_STATE previousPad_ = {};
  // スティック用デッドゾーン（左右別）
  short leftThumbDeadZone_ = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;   // 7849
  short rightThumbDeadZone_ = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE; // 8689

  // --- 環境情報 ---
  bool isInitialized_ = false;
  HWND hwnd_ = nullptr;
};
