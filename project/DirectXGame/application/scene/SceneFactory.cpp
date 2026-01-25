#include "SceneFactory.h"

#include "GameScene.h"
#include "SceneIds.h"
#include "TitleScene.h"

std::unique_ptr<BaseScene> SceneFactory::Create(const std::string &sceneId,
                                                const SceneServices &services) {
  (void)services;

  if (sceneId == SceneId::Title) {
    return std::make_unique<TitleScene>();
  }
  if (sceneId == SceneId::Game) {
    return std::make_unique<GameScene>();
  }
  return nullptr;
}
