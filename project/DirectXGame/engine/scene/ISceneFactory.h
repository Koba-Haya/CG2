#pragma once
#include <memory>
#include <string>

class BaseScene;
struct SceneServices;

class ISceneFactory {
public:
  virtual ~ISceneFactory() = default;

  // sceneId からシーン生成。存在しないなら nullptr を返す
  virtual std::unique_ptr<BaseScene> Create(const std::string &sceneId,
                                            const SceneServices &services) = 0;
};
