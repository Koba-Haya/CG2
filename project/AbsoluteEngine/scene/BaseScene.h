#pragma once
#include <string>
#include <memory>
#include <vector>

#include "SceneServices.h"
#include "timeline/TimelineManager.h"
#include "timeline/TimelineEditorWindow.h"

class Camera;
class GameCamera;
namespace AbsoluteEngine {
    class GameObject;
    class EditorUIManager;
    class EditorCamera;
    class CommandManager;
}

class SceneManager;

enum class PlayMode {
  Edit,
  Play,
  Pause
};

class BaseScene {
public:
  virtual ~BaseScene() = default;

  void SetSceneManager(SceneManager *sm) { sceneManager_ = sm; }
  void SetSceneId(const std::string& id) { sceneId_ = id; }

  virtual void Initialize(const SceneServices &services);
  virtual void Finalize() {}
  virtual void Update() {}
  virtual void Draw() = 0;
  virtual void DrawEditorUI();

  PlayMode GetPlayMode() const { return playMode_; }
  void SetPlayMode(PlayMode mode) { playMode_ = mode; }

  const std::vector<std::shared_ptr<AbsoluteEngine::GameObject>>& GetRootObjects() const { return rootObjects_; }
  // タイムラインのSpawnEventなど、あらゆる箇所からのオブジェクト追加を一元管理できるよう virtual 化する
  // 派生クラスでオーバーライドすることで、Application固有の処理（コールバック注入など）を追加できる（DI）
  virtual void AddRootObject(std::shared_ptr<AbsoluteEngine::GameObject> obj) { rootObjects_.push_back(obj); }
  void ClearRootObjects() { rootObjects_.clear(); }

  // アクティブシーンへのグローバルアクセス
  static BaseScene* GetActiveScene() { return activeScene_; }

  AbsoluteEngine::CommandManager* GetCommandManager() const;

  // エンジンインフラ機能のグローバルアクセス
  GameCamera* GetMainCamera() const { return mainCamera_.get(); }
  void SetMainCamera(std::shared_ptr<GameCamera> camera) { mainCamera_ = camera; }
  class Input* GetInput() const { return services_.input; }

protected:
  virtual void BackupScene();
  virtual void RestoreScene();
  virtual void SaveEditorScene();
  bool LoadEditorScene();
  std::string GetSceneFilePath() const;

  // タイムラインデータのファイルパスを取得する（sceneId_ + "_timeline.json"）
  std::string GetTimelineFilePath() const;

  // タイムラインデータを保存・読み込みする
  void SaveTimeline();
  void LoadTimeline();

  // タイムラインのカメラを設定する（RailCameraComponentへの参照を渡す）
  // 派生クラスのInitializeでRailCameraComponentを設定後に呼ぶ
  void SetTimelineRailCamera(class RailCameraComponent* camera);

  // タイムラインを更新する（プレイモード中に毎フレーム呼ぶ）
  void UpdateTimeline(float deltaTime);

  // タイムラインのエディタUIを描画する
  void DrawTimelineEditorUI();

protected:
  void RequestSceneChange(const std::string &sceneId);

  // エディタ機能の更新・描画（派生クラスのUpdate/Drawから呼ばれることを想定）
  virtual void UpdateEditor();
  void ApplyEditorLightsToRenderer(class Renderer* renderer);

  // editorCamera_（Debug/Free Camera）による入力操作を有効にするかどうか（タスクD）
  // GameScene 側の「Debug Camera」トグルと描画分岐を一致させるため、
  // 派生クラスでオーバーライドしてトグル状態を返すことで、
  // OFF時は非表示のeditorCamera_が入力を奪わないようにする
  virtual bool IsEditorCameraOperationEnabled() const { return true; }

protected:
  SceneManager *sceneManager_ = nullptr;
  SceneServices services_{};

  // エンジン・エディタ機能
  std::unique_ptr<AbsoluteEngine::EditorUIManager> editorUIManager_;
  std::vector<std::shared_ptr<AbsoluteEngine::GameObject>> rootObjects_;
  std::unique_ptr<Camera> editorCamera_; // エディタ用カメラ
  std::shared_ptr<GameCamera> mainCamera_; // ゲーム全体で共有するメインカメラ

  PlayMode playMode_ = PlayMode::Edit;
  std::string sceneId_ = "";
  std::string backupSceneJson_ = "";

  // タイムラインシステム（シーン独立のため BaseScene が所有する）
  AbsoluteEngine::TimelineManager timelineManager_;
  AbsoluteEngine::TimelineEditorWindow timelineEditorWindow_;

  // タイムラインプレビューオブジェクト（タスク16）
  // イベント選択時に生成した仮のオブジェクトへの弱参照
  // 他のイベントへ選択が切り替わった際は古いプレビューを Destroy()してから新規生成する
  std::weak_ptr<AbsoluteEngine::GameObject> timelinePreviewObject_;

  // 前フレームの選択イベントポインタ（選択切り替え検出用）を保持する
  AbsoluteEngine::ITimelineEvent* prevSelectedTimelineEvent_ = nullptr;

  static BaseScene* activeScene_;
};
