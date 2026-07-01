#include "TextureManager.h"
#include "TextureResource.h"
#include <Windows.h>

#include <filesystem>
#include "../../base/EnginePath.h"
static void TexFatal_(const std::string &msg) {
  MessageBoxA(nullptr, msg.c_str(), "Texture Fatal", MB_OK | MB_ICONERROR);
}

static std::string ResolveTexPath_(const std::string& path) {
  if (std::filesystem::exists(path)) return path;
  std::string altPath = AbsoluteEngine::EnginePath::Resolve(path);
  if (std::filesystem::exists(altPath)) return altPath;
  return path;
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

  const std::string resolvedPath = ResolveTexPath_(path);
  const bool ok = tex->CreateFromFile(dx_, resolvedPath);
  if (!ok) {
    TexFatal_(std::string("[TextureManager] CreateFromFile failed:\n") + resolvedPath);
    return nullptr;
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
