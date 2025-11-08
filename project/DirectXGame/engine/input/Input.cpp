#include "Input.h"
#include <cassert>
#include <cstring> // std::memcpy, std::memset

//--------------------------------------------------------------------------------------
// コンストラクタ / デストラクタ
//--------------------------------------------------------------------------------------
Input::Input() {}
Input::~Input() { Finalize(); }

//--------------------------------------------------------------------------------------
// Initialize: DirectInput を作成し、キーボード＆マウスのデバイスを準備
//--------------------------------------------------------------------------------------
bool Input::Initialize(HINSTANCE instanceHandle, HWND windowHandle) {
  hwnd_ = windowHandle;

  // 1) DirectInput8 の本体インターフェースを作成
  HRESULT hr = DirectInput8Create(
      instanceHandle, DIRECTINPUT_VERSION, IID_IDirectInput8,
      reinterpret_cast<void **>(directInput_.GetAddressOf()), nullptr);
  if (FAILED(hr)) {
    return false;
  }

  // 2) キーボードデバイスを作成
  hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, nullptr);
  if (FAILED(hr)) {
    return false;
  }

  // 3) キーボードのデータ形式を「キーボード用の既定フォーマット」に設定
  hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
  if (FAILED(hr)) {
    return false;
  }

  // 4) キーボードの協調レベルを設定
  //    - DISCL_FOREGROUND   : ウィンドウがフォアグラウンドの時だけ入力
  //    - DISCL_NONEXCLUSIVE : 他アプリと共有（排他しない）
  //    - DISCL_NOWINKEY     : Windowsキーをアプリ内で無効化（誤操作防止）
  hr = keyboard_->SetCooperativeLevel(
      hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  if (FAILED(hr)) {
    return false;
  }

  // 5) マウスデバイスを作成（GUID_SysMouse）
  hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, nullptr);
  if (FAILED(hr)) {
    return false;
  }

  // 6)
  // マウスのデータ形式を「DIMOUSESTATE2」に設定（ホイールや5ボタン以上に対応）
  hr = mouse_->SetDataFormat(&c_dfDIMouse2);
  if (FAILED(hr)) {
    return false;
  }

  // 7) マウスの協調レベル
  hr =
      mouse_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
  if (FAILED(hr)) {
    return false;
  }

  // 8) 初期バッファクリア（前フレーム = 0）
  std::memset(currentKeys_, 0, sizeof(currentKeys_));
  std::memset(previousKeys_, 0, sizeof(previousKeys_));
  std::memset(&currentMouse_, 0, sizeof(currentMouse_));
  std::memset(&previousMouse_, 0, sizeof(previousMouse_));
  std::memset(&currentPad_, 0, sizeof(currentPad_));
  std::memset(&previousPad_, 0, sizeof(previousPad_));

  isInitialized_ = true;
  return true;
}

//--------------------------------------------------------------------------------------
// Finalize: DirectInput デバイスを解放
//--------------------------------------------------------------------------------------
void Input::Finalize() {
  // マウスを最初に解放
  if (mouse_) {
    mouse_->Unacquire(); // 入力の取得状態を解除
    mouse_->Release();   // COM 参照カウントをデクリメント
    mouse_ = nullptr;
  }

  // キーボード解放
  if (keyboard_) {
    keyboard_->Unacquire();
    keyboard_->Release();
    keyboard_ = nullptr;
  }

  // DirectInput 本体解放
  if (directInput_) {
    directInput_->Release();
    directInput_ = nullptr;
  }

  isInitialized_ = false;
}

//--------------------------------------------------------------------------------------
// Update: 各デバイスから状態を取得し、前フレームとの差分を保管
//--------------------------------------------------------------------------------------
void Input::Update() {
  if (!isInitialized_) {
    return;
  }

  //====================
  // キーボード
  //====================
  // 1) 前フレームのキー配列を退避
  std::memcpy(previousKeys_, currentKeys_, sizeof(currentKeys_));

  // 2) いったん現フレーム配列を 0 で初期化（安全策）
  std::memset(currentKeys_, 0, sizeof(currentKeys_));

  // 3) Acquire（入力デバイスを占有）。Alt+Tab
  // 復帰などで失敗する場合があるためリトライ関数に分離
  TryAcquireKeyboard_();

  // 4) 状態を 256 バイト配列へ取得（0x80 立っていれば押下中）
  HRESULT hr = keyboard_->GetDeviceState(sizeof(currentKeys_), currentKeys_);
  if (FAILED(hr)) {
    // 失敗時はデバイスロスト扱い→再 Acquire を試み、もう一度取得
    TryAcquireKeyboard_();
    keyboard_->GetDeviceState(sizeof(currentKeys_), currentKeys_);
  }

  //====================
  // マウス
  //====================
  // 1) 前フレームのマウス状態を退避
  previousMouse_ = currentMouse_;

  // 2) 現フレームのバッファを 0 クリア
  std::memset(&currentMouse_, 0, sizeof(currentMouse_));

  // 3) Acquire と状態取得
  TryAcquireMouse_();
  hr = mouse_->GetDeviceState(sizeof(DIMOUSESTATE2), &currentMouse_);
  if (FAILED(hr)) {
    // 失敗時は再 Acquire → もう一度取得
    TryAcquireMouse_();
    mouse_->GetDeviceState(sizeof(DIMOUSESTATE2), &currentMouse_);
  }

  //====================
  // ゲームパッド（XInput）
  //====================
  // 1) 前フレーム状態を退避
  previousPad_ = currentPad_;

  // 2) 現フレーム状態を 0 で初期化
  std::memset(&currentPad_, 0, sizeof(currentPad_));

  // 3) XInput から読み出し（内部でデッドゾーン適用）
  ReadGamepad_();
}

//--------------------------------------------------------------------------------------
// キーボード：問い合わせ系
//--------------------------------------------------------------------------------------
bool Input::IsDown(int dik) const { return (currentKeys_[dik] & 0x80) != 0; }

bool Input::WasPressed(int dik) const {
  const bool now = (currentKeys_[dik] & 0x80) != 0;
  const bool prev = (previousKeys_[dik] & 0x80) != 0;
  return now && !prev;
}

bool Input::WasReleased(int dik) const {
  const bool now = (currentKeys_[dik] & 0x80) != 0;
  const bool prev = (previousKeys_[dik] & 0x80) != 0;
  return !now && prev;
}

const BYTE *Input::GetKeyboardState() const { return currentKeys_; }

//--------------------------------------------------------------------------------------
// マウス：問い合わせ系
//--------------------------------------------------------------------------------------
Input::MouseState Input::GetMouse() const {
  MouseState s{};
  s.dx = currentMouse_.lX;    // 相対 X 移動（1フレーム分）
  s.dy = currentMouse_.lY;    // 相対 Y 移動
  s.wheel = currentMouse_.lZ; // ホイール（前+ 後-）

  // rgbButtons[0..7] のビット7 が押下フラグ（0x80）
  for (int i = 0; i < 8; ++i) {
    s.buttons[i] = currentMouse_.rgbButtons[i];
  }
  return s;
}

bool Input::IsMouseDown(int buttonIndex) const {
  if (buttonIndex < 0 || buttonIndex >= 8) {
    return false;
  }
  return (currentMouse_.rgbButtons[buttonIndex] & 0x80) != 0;
}

bool Input::WasMousePressed(int buttonIndex) const {
  if (buttonIndex < 0 || buttonIndex >= 8) {
    return false;
  }
  const bool now = (currentMouse_.rgbButtons[buttonIndex] & 0x80) != 0;
  const bool prev = (previousMouse_.rgbButtons[buttonIndex] & 0x80) != 0;
  return now && !prev;
}

bool Input::WasMouseReleased(int buttonIndex) const {
  if (buttonIndex < 0 || buttonIndex >= 8) {
    return false;
  }
  const bool now = (currentMouse_.rgbButtons[buttonIndex] & 0x80) != 0;
  const bool prev = (previousMouse_.rgbButtons[buttonIndex] & 0x80) != 0;
  return !now && prev;
}

//--------------------------------------------------------------------------------------
// ゲームパッド：問い合わせ系
//--------------------------------------------------------------------------------------
bool Input::IsGamepadConnected() const { return gamepadConnected_; }

Input::GamepadState Input::GetGamepad() const {
  GamepadState s{};
  s.buttons = currentPad_.Gamepad.wButtons;
  s.leftTrigger = currentPad_.Gamepad.bLeftTrigger;
  s.rightTrigger = currentPad_.Gamepad.bRightTrigger;
  s.lx = currentPad_.Gamepad.sThumbLX;
  s.ly = currentPad_.Gamepad.sThumbLY;
  s.rx = currentPad_.Gamepad.sThumbRX;
  s.ry = currentPad_.Gamepad.sThumbRY;
  return s;
}

bool Input::IsPadDown(WORD xinputButton) const {
  return (currentPad_.Gamepad.wButtons & xinputButton) != 0;
}

bool Input::WasPadPressed(WORD xinputButton) const {
  const bool now = (currentPad_.Gamepad.wButtons & xinputButton) != 0;
  const bool prev = (previousPad_.Gamepad.wButtons & xinputButton) != 0;
  return now && !prev;
}

bool Input::WasPadReleased(WORD xinputButton) const {
  const bool now = (currentPad_.Gamepad.wButtons & xinputButton) != 0;
  const bool prev = (previousPad_.Gamepad.wButtons & xinputButton) != 0;
  return !now && prev;
}

void Input::SetThumbDeadZone(short leftDead, short rightDead) {
  leftThumbDeadZone_ = (leftDead < 0) ? 0 : leftDead;
  rightThumbDeadZone_ = (rightDead < 0) ? 0 : rightDead;
}

//--------------------------------------------------------------------------------------
// 内部：キーボード・マウスの Acquire（入力デバイスの再取得）
//--------------------------------------------------------------------------------------
void Input::TryAcquireKeyboard_() {
  // 失敗しても無限ループしないように、戻り値は捨てる
  if (keyboard_) {
    keyboard_->Acquire();
  }
}

void Input::TryAcquireMouse_() {
  if (mouse_) {
    mouse_->Acquire();
  }
}

//--------------------------------------------------------------------------------------
// 内部：XInput 読み出し＋デッドゾーン適用
//--------------------------------------------------------------------------------------
void Input::ReadGamepad_() {
  // 0番スロット（1台目のコントローラ）から状態取得
  DWORD dwResult = XInputGetState(0, &currentPad_);
  gamepadConnected_ = (dwResult == ERROR_SUCCESS);

  if (!gamepadConnected_) {
    // 未接続なら currentPad_ は 0 のまま（問い合わせは全て false になる）
    std::memset(&currentPad_, 0, sizeof(currentPad_));
    return;
  }

  // スティックのデッドゾーン処理（中心付近の微小揺れを 0 に丸める）
  auto applyDeadZone = [](SHORT v, SHORT dead) -> SHORT {
    if (v > -dead && v < dead) {
      return 0;
    }
    return v;
  };
  currentPad_.Gamepad.sThumbLX =
      applyDeadZone(currentPad_.Gamepad.sThumbLX, leftThumbDeadZone_);
  currentPad_.Gamepad.sThumbLY =
      applyDeadZone(currentPad_.Gamepad.sThumbLY, leftThumbDeadZone_);
  currentPad_.Gamepad.sThumbRX =
      applyDeadZone(currentPad_.Gamepad.sThumbRX, rightThumbDeadZone_);
  currentPad_.Gamepad.sThumbRY =
      applyDeadZone(currentPad_.Gamepad.sThumbRY, rightThumbDeadZone_);

  // トリガのしきい値（既定は 30 程度が多い）。ここでは 0
  // のままにして問い合わせ側で自由に扱えるようにする。
  // 必要ならここで最小しきい値未満を 0 に丸める処理を追加可能。
}
