#include "ModelManager.h"

#include <cassert>

#include "AssetLoader.h"
#include "ModelResource.h"

std::shared_ptr<ModelResource> ModelManager::Load(const std::string &path) {
  assert(dx_);

  if (auto it = cache_.find(path); it != cache_.end()) {
    if (auto alive = it->second.lock()) {
      return alive;
    }
  }

  auto md = AssetLoader::GetInstance()->LoadModel(path);
  assert(md);

  auto res = std::make_shared<ModelResource>();

  ModelResource::CreateInfo ci{};
  ci.dx = dx_;
  ci.modelData = md;

  const bool ok = res->Initialize(ci);
  assert(ok);

  cache_[path] = res;
  return res;
}

void ModelManager::ClearUnused() {
  for (auto it = cache_.begin(); it != cache_.end();) {
    if (it->second.expired()) {
      it = cache_.erase(it);
    } else {
      ++it;
    }
  }
}
