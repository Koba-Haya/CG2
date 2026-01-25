#pragma once
#include <memory>
#include <string>

#include "ISceneFactory.h"

class SceneFactory final : public ISceneFactory {
public:
  std::unique_ptr<BaseScene> Create(const std::string &sceneId,
                                    const SceneServices &services) override;
};
