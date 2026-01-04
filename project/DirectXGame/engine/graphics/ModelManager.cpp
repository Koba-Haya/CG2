#include "ModelManager.h"
#include "ModelResource.h"
#include "ModelUtils.h"
#include <cassert>

std::shared_ptr<ModelResource>
ModelManager::LoadObj(const std::string& directoryPath,
    const std::string& filename) {
    assert(dx_);
    assert(srvAlloc_);

    const std::string key = MakeKey_(directoryPath, filename);

    if (auto it = cache_.find(key); it != cache_.end()) {
        if (auto alive = it->second.lock()) {
            return alive;
        }
    }

    ModelData data = LoadObjFile(directoryPath, filename);

    auto res = std::make_shared<ModelResource>();
    ModelResource::CreateInfo ci{};
    ci.dx = dx_;
    ci.modelData = std::move(data);

    const bool ok = res->Initialize(ci);
    assert(ok);

    cache_[key] = res;
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
