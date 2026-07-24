#pragma once
#include "BaseScene.h"
#include "GameCamera.h"
#include "Skybox.h"
#include "DebugCamera.h"
#include "ModelInstance.h"
#include "ModelResource.h"
#include "Renderer.h"
#include "../actor/Player/PlayerComponent.h"
#include "../actor/Bullet/BulletComponent.h"
#include "../actor/Enemy/EnemyShootComponent.h"
#include "../hud/GameHUD.h"
#include "AbsoluteEngine/resources/AssetManager.h"
#include "TextureResource.h"
#include <memory>
#include <vector>
#include <string>

enum class GamePhase {
  InProgress,
  Boss,
  Clear,
  GameOver
};

// -----------------------------------------------------------------------
// SpawnEvent 構造体
// 将来のタイムラインエディタ（再生側）の基盤となるウェーブデータ単位。
// -----------------------------------------------------------------------
struct SpawnEvent {
    float triggerTime;    // 出現時間（ゲーム開始からの秒数）
    std::string prefabId; // 敵の種類ID（"Enemy" など）
    Vector3 position;     // 出現ワールド座標
    bool spawned = false; // 既にスポーン済みかどうか（内部管理用）
};

class GameScene final : public BaseScene {
public:
  GameScene() = default;
  ~GameScene() override = default;

  void Initialize(const SceneServices &services) override;
  void Finalize() override;
  void Update() override;
  void Draw() override;
  void DrawEditorUI() override;

  // AddRootObject をオーバーライドして EnemyComponent へのコールバックを自動注入する（タスクC）
  // SpawnEvent 経由でスポーンされた敵にも onDestroyed が登録されるようになる
  void AddRootObject(std::shared_ptr<AbsoluteEngine::GameObject> obj) override;

protected:
  // Debug Camera トグルがONの間だけ editorCamera_ の入力操作を許可する（タスクD）
  bool IsEditorCameraOperationEnabled() const override { return isDebugCamera_; }

  // Timeline Editor内のDebug CameraトグルボタンにisDebugCamera_のアドレスを渡す
  bool* GetDebugCameraFlag() override { return &isDebugCamera_; }

  // Draw() の描画カメラ分岐と一致させる（ギズモ/マウスピッキングが実際の見た目とズレないように）
  Camera* GetEditorViewCamera() const override;

private:
  // ヒットエフェクトの発生（パーティクル＋リングエフェクト＋爆発ライト）
  void SpawnHitEffect(const Vector3 &pos);

  // ウェーブデータの初期化（デモ用ハードコードデータを登録する）
  //void InitSpawnEvents_();

private:
  std::unique_ptr<DebugCamera> debugCamera_;

  bool isDebugCamera_ = false;
  bool showDebugRail_ = true;

  std::string sceneFilePath_ = "C:/Users/haya2/source/repos/CG2/project/Application/resources/editor/scene.json";

  Skybox skybox_;

  // アクター関連
  std::shared_ptr<AbsoluteEngine::GameObject> playerObj_;

  // リソース
  std::shared_ptr<ModelResource> resPlayer_;
  std::shared_ptr<ModelResource> resBullet_;
  std::shared_ptr<ModelResource> resEnemy_;
  std::shared_ptr<ModelResource> resEffect_;

  // ヒットエフェクト用テクスチャ
  std::shared_ptr<TextureResource> texRing_;
  std::shared_ptr<TextureResource> texNoise0_;

  // ヒットエフェクト（旧型：スケールアニメーション）
  struct HitEffect {
    HitEffect() = default;
    ~HitEffect() = default;
    HitEffect(HitEffect &&) noexcept = default;
    HitEffect &operator=(HitEffect &&) noexcept = default;

    ModelInstance instance;
    float frame = 0.0f;
    float maxFrame = 20.0f;
    bool isActive = false;
    Vector3 position;
  };
  std::vector<HitEffect> hitEffects_;

  // カメラシェイク用
  float cameraShakeTimer_ = 0.0f;
  float cameraShakeDuration_ = 0.0f;
  float cameraShakeIntensity_ = 0.0f;
  Vector3 cameraShakeOffset_{ 0, 0, 0 };

  // 画面歪み（RadialBlur）用
  float hitDistortionTimer_ = 0.0f;
  float hitDistortionDuration_ = 0.0f;
  float hitDistortionIntensity_ = 0.0f;
  Vector2 radialBlurCenter_ = { 0.5f, 0.5f };

  // -----------------------------------------------------------------------
  // SpawnManager：データ駆動型の簡易ウェーブシステム
  // -----------------------------------------------------------------------
  float spawnTimer_ = 0.0f;           // ゲーム開始からの経過時間（スポーン判定用）
  std::vector<SpawnEvent> spawnEvents_; // 全スポーンイベントリスト

  float time_ = 0.0f;

  GamePhase phase_ = GamePhase::InProgress;

  std::unique_ptr<GameHUD> gameHUD_;
};