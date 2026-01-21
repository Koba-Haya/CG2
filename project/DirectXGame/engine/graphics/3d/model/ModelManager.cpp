#include "ModelManager.h"
#include "AssetLoader.h"
#include "ModelResource.h"
#include <cassert>

std::shared_ptr<ModelResource>
ModelManager::LoadObj(const std::string &directoryPath,
                      const std::string &filename) {
  assert(dx_);
  assert(srvAlloc_);

  const std::string key = MakeKey_(directoryPath, filename);

  if (auto it = cache_.find(key); it != cache_.end()) {
    if (auto alive = it->second.lock()) {
      return alive;
    }
  }

  // Assimp（OBJもglTFも読める）
  const ModelData &loaded =
      AssetLoader::GetInstance()->LoadModel(directoryPath, filename);

  auto res = std::make_shared<ModelResource>();
  ModelResource::CreateInfo ci{};
  ci.dx = dx_;
  ci.modelData =
      loaded; // キャッシュ内参照からコピー（中身が大きいなら参照共有設計に後で変える）

  const bool ok = res->Initialize(ci);
  assert(ok);

  cache_[key] = res;
  return res;
}
