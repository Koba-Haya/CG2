#pragma once
#include <string>

#include "SceneServices.h"

class SceneManager;

class BaseScene {
public:
  virtual ~BaseScene() = default;

  void SetSceneManager(SceneManager *sm) { sceneManager_ = sm; }

  virtual void Initialize(const SceneServices &services) {
    services_ = services;
  }
  virtual void Finalize() {}
  virtual void Update() {}
  virtual void Draw() = 0;

protected:
  void RequestSceneChange(const std::string &sceneId);

protected:
  SceneManager *sceneManager_ = nullptr;
  SceneServices services_{};
};
