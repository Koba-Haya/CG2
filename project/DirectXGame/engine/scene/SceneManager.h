#pragma once
#include <d3d12.h>
#include <memory>
#include <string>

#include "SceneServices.h"
#include "BaseScene.h"
#include "ISceneFactory.h"

class SceneManager {
public:
  SceneManager() = default;
  ~SceneManager();

  void Initialize(const SceneServices &services);
  void Finalize();

  void Update();
  void Draw();

  void SetFactory(std::unique_ptr<ISceneFactory> factory);
  void Start(const std::string &firstSceneId);

  void RequestChange(const std::string &nextSceneId);
  void ApplySceneChangeIfNeeded();

  const SceneServices &GetServices() const { return services_; }

private:
  void SetNextSceneById_(const std::string &sceneId);

private:
  SceneServices services_{};
  std::unique_ptr<ISceneFactory> factory_;
  std::unique_ptr<BaseScene> current_;
  std::unique_ptr<BaseScene> next_;
  bool hasPendingChange_ = false;
  std::string pendingSceneId_;
};
