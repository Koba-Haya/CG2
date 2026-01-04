#include "TextureManager.h"
#include "TextureResource.h"
#include <cassert>

std::shared_ptr<TextureResource> TextureManager::Load(const std::string& path) {
    assert(dx_);

    if (auto it = cache_.find(path); it != cache_.end()) {
        if (auto alive = it->second.lock()) {
            return alive;
        }
    }

    auto tex = std::make_shared<TextureResource>();
    const bool ok = tex->CreateFromFile(dx_, path);
    assert(ok);

    cache_[path] = tex;
    return tex;
}

void TextureManager::ClearUnused() {
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (it->second.expired()) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}
