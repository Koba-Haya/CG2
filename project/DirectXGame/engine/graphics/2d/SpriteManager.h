#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class SpriteResource;

class SpriteManager {
public:
  static SpriteManager *GetInstance() {
    static SpriteManager inst;
    return &inst;
  }

  std::shared_ptr<SpriteResource> Load(const std::string &path);
  void ClearUnused();

private:
  SpriteManager() = default;
  std::unordered_map<std::string, std::weak_ptr<SpriteResource>> cache_;
};