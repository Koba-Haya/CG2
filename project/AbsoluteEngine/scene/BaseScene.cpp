#include "BaseScene.h"
#include "SceneManager.h"

void BaseScene::RequestSceneChange(const std::string &sceneId) {
  if (sceneManager_) {
    sceneManager_->RequestChange(sceneId);
  }
}
