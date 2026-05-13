#pragma once

#include <string>
#include <map>
#include <memory>
#include "Animation.h"

class AnimationManager {
public:
  static AnimationManager* GetInstance() {
    static AnimationManager inst;
    return &inst;
  }

  std::shared_ptr<Animation> LoadAnimation(const std::string& directoryPath, const std::string& filename);

  void Clear() { cache_.clear(); }

private:
  AnimationManager() = default;

  std::map<std::string, std::shared_ptr<Animation>> cache_;
};
