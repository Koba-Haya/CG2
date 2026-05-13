#include "TextureManager.h"
#include "TextureResource.h"
#include <Windows.h>

static void TexFatal_(const std::string &msg) {
  MessageBoxA(nullptr, msg.c_str(), "Texture Fatal", MB_OK | MB_ICONERROR);
  std::terminate();
}

std::shared_ptr<TextureResource> TextureManager::Load(const std::string &path) {
  if (!dx_) {
    TexFatal_("[TextureManager] dx_ is null. Call Initialize first.");
  }

  if (auto it = cache_.find(path); it != cache_.end()) {
    if (auto alive = it->second.lock()) {
      return alive;
    }
  }

  auto tex = std::make_shared<TextureResource>();

  const bool ok = tex->CreateFromFile(dx_, path);
  if (!ok) {
    TexFatal_(std::string("[TextureManager] CreateFromFile failed:\n") + path);
  }

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
