#pragma once
#include <string>
#include <memory>
#include <vector>

#include "SceneServices.h"

class Camera;
namespace AbsoluteEngine {
    class GameObject;
    class EditorUIManager;
    class EditorCamera;
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

  virtual void Initialize(const SceneServices &services);
  virtual void Finalize() {}
  virtual void Update() {}
  virtual void Draw() = 0;

  PlayMode GetPlayMode() const { return playMode_; }
  void SetPlayMode(PlayMode mode) { playMode_ = mode; }

  const std::vector<std::shared_ptr<AbsoluteEngine::GameObject>>& GetRootObjects() const { return rootObjects_; }
  void AddRootObject(std::shared_ptr<AbsoluteEngine::GameObject> obj) { rootObjects_.push_back(obj); }
  void ClearRootObjects() { rootObjects_.clear(); }

protected:
  void RequestSceneChange(const std::string &sceneId);

  // エディタ機能の更新・描画（派生クラスのUpdate/Drawから呼ばれることを想定）
  virtual void UpdateEditor();
  virtual void DrawEditorUI();
  void ApplyEditorLightsToRenderer(class Renderer* renderer);

protected:
  SceneManager *sceneManager_ = nullptr;
  SceneServices services_{};

  // エンジン・エディタ機能
  std::unique_ptr<AbsoluteEngine::EditorUIManager> editorUIManager_;
  std::vector<std::shared_ptr<AbsoluteEngine::GameObject>> rootObjects_;
  std::unique_ptr<Camera> editorCamera_; // エディタ用カメラ

  PlayMode playMode_ = PlayMode::Edit;
  std::string backupSceneJson_ = "";
};
