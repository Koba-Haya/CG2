#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "Type/Vector.h"
#include "LockOnSystem.h"
#include <memory>

class Input;
class GameCamera;

class PlayerComponent : public AbsoluteEngine::IComponent {
public:
  PlayerComponent() = default;
  ~PlayerComponent() override = default;


  LockOnSystem lockon_; // 外部からアクセス可能にする

  void Initialize();
  void Update(float deltaTime) override;
  void OnCollision(AbsoluteEngine::GameObject* other) override;

  std::string GetTypeName() const override { return "PlayerComponent"; }

  void TakeDamage(int damage);
  bool IsDead() const;
  int GetHp() const { return hp_; }
  int GetMaxHp() const { return maxHp_; }

  // カーソル（照準）のスクリーン座標を取得
  const Vector2& GetCursorPos() const { return cursorPos_; }

  // 3D レティクルの各距離のワールド座標を取得（GameHUD・弾道計算に共有）
  const Vector3& GetNearReticleWorldPos() const { return nearReticleWorldPos_; }
  const Vector3& GetMidReticleWorldPos()  const { return midReticleWorldPos_; }
  const Vector3& GetFarReticleWorldPos()  const { return farReticleWorldPos_; }

private:
  Vector2 cursorPos_{ 640.0f, 360.0f }; // カーソルのスクリーン座標
  float cursorSpeed_ = 1000.0f;          // カーソルの移動速度（ピクセル/秒）：キビキビした操作感のため引き上げ

  Vector2 localPos_{ 0.0f, -0.5f }; // 自機の目標ローカル座標への追従用
  float moveSpeed_ = 4.0f;          // 追従速度（Lerpの係数）：スターフォックス64風に大幅引き上げ
  float cameraDistance_ = 22.0f;    // カメラから自機までの前方距離

  // バンク角制御（原作ライクなロール・ピッチ傾き）
  float bankRoll_  = 0.0f; // 現在のロール角（ラジアン）
  float bankPitch_ = 0.0f; // 現在のピッチ上乗せ角（ラジアン）

  // 3D レティクルの各距離のワールド座標（毎フレーム計算してキャッシュ）
  Vector3 nearReticleWorldPos_{ 0, 0, 0 };
  Vector3 midReticleWorldPos_ { 0, 0, 0 };
  Vector3 farReticleWorldPos_ { 0, 0, 0 };

  // カーソル方向のカメラ射線ベクトルキャッシュ（毎フレーム更新）
  // 弾の発射方向に直接使用することで、
  // t.translateのLerp遅れによる方向ズレを完全に排除する。
  Vector3 camDirCached_{ 0, 0, 1 }; // 初期値は正面方向

  int hp_ = 5;
  int maxHp_ = 5;
  float shootCooldown_ = 0.0f;
};
