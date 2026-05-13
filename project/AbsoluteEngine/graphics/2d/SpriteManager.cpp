#include "SpriteManager.h"
#include "SpriteResource.h"

std::shared_ptr<SpriteResource> SpriteManager::Load(const std::string &path) {
  if (auto it = cache_.find(path); it != cache_.end()) {
    if (auto alive = it->second.lock())
      return alive;
  }

  auto res = std::make_shared<SpriteResource>();
  if (!res->Initialize(path))
    return nullptr;

  cache_[path] = res;
  return res;
}

void SpriteManager::ClearUnused() {
  for (auto it = cache_.begin(); it != cache_.end();) {
    if (it->second.expired())
      it = cache_.erase(it);
    else
      ++it;
  }
}