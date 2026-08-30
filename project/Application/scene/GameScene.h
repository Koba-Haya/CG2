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
#include "Animation.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

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

  // EnemyComponent::onDestroyed コールバックの登録を1箇所に集約する。
  // 以前は Initialize / AddRootObject / Update(SpawnManager) の3箇所に
  // 全く同じ内容のラムダが重複していたため、ここに統合した。
  // FormationMemberComponentを持つ場合は編隊トラッキングの初期カウントも行う。
  void RegisterEnemyCallbacks(const std::shared_ptr<AbsoluteEngine::GameObject>& obj);

  // 編隊メンバーが1体撃破されるたびに呼ばれる。残数が0になったら編隊全滅ボーナスを発火する。
  void HandleFormationMemberDestroyed(int formationId, const Vector3& pos);

  // 編隊全滅ボーナス（スコア倍率＋強化演出）
  void OnFormationCleared(const Vector3& pos, int memberCount);

private:
  std::unique_ptr<DebugCamera> debugCamera_;

  bool isDebugCamera_ = false;
  bool showDebugRail_ = true;
  bool showDebugSkeleton_ = false; // Bキーでトグル：プレイヤーの骨をデバッグ表示

  std::string sceneFilePath_ = "C:/Users/haya2/source/repos/CG2/project/Application/resources/editor/scene.json";

  Skybox skybox_;

  // アクター関連
  std::shared_ptr<AbsoluteEngine::GameObject> playerObj_;

  // リソース
  std::shared_ptr<ModelResource> resPlayer_;
  std::shared_ptr<ModelResource> resBullet_;
  std::shared_ptr<ModelResource> resEnemy_;
  std::shared_ptr<ModelResource> resEffect_;

  // プレイヤー用スキニングモデル（歩行アニメーション）
  std::shared_ptr<Animation> playerWalkAnim_;
  // ロックオン中に切り替える別アニメーション（Animation補間の実演用）
  std::shared_ptr<Animation> playerSneakWalkAnim_;
  bool wasLockingMode_ = false;

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

  // ボス出現演出（GaussianFilter）用
  float bossIntroBlurTimer_ = 0.0f;
  float bossIntroBlurDuration_ = 0.0f;

  // 被弾演出（Vignette）用
  float playerHitVignetteTimer_ = 0.0f;
  float playerHitVignetteDuration_ = 0.0f;
  int lastPlayerHp_ = -1; // 前フレームのHP。減少を検知したらVignetteを発火させる

  // 敵出現演出（BoxFilter）用：SpawnEventで新しい敵が出現した瞬間に発火
  float waveSpawnBlurTimer_ = 0.0f;
  float waveSpawnBlurDuration_ = 0.0f;

  // キルストリーク演出（Random）用：一定数の敵を撃破するごとに発火
  float killStreakGlitchTimer_ = 0.0f;
  float killStreakGlitchDuration_ = 0.0f;
  int killCount_ = 0;

  // -----------------------------------------------------------------------
  // SpawnManager：データ駆動型の簡易ウェーブシステム
  // -----------------------------------------------------------------------
  float spawnTimer_ = 0.0f;           // ゲーム開始からの経過時間（スポーン判定用）
  std::vector<SpawnEvent> spawnEvents_; // 全スポーンイベントリスト

  // -----------------------------------------------------------------------
  // 編隊（Formation）トラッキング用
  // -----------------------------------------------------------------------
  // 編隊ID -> 残存メンバー数。0になった時点でOnFormationCleared()を発火してmapから削除する
  std::unordered_map<int, int> formationRemaining_;
  // 編隊ID -> 初期メンバー数（ボーナス計算用。全滅時にformationRemaining_と一緒に削除する）
  std::unordered_map<int, int> formationTotal_;

  // スコア（内部カウンタのみ。本番UI表示は別タスク。現状はImGuiデバッグ表示で仮置き）
  int score_ = 0;

  float time_ = 0.0f;

  GamePhase phase_ = GamePhase::InProgress;

  std::unique_ptr<GameHUD> gameHUD_;
};