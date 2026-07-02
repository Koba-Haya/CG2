#include "AssetManager.h"
#include "../graphics/3d/model/ModelManager.h"
#include "../graphics/texture/TextureManager.h"

namespace AbsoluteEngine {

template <>
std::shared_ptr<ModelResource> AssetManager::Load<ModelResource>(const std::string& path) {
    return ModelManager::GetInstance()->Load(path);
}

template <>
std::shared_ptr<TextureResource> AssetManager::Load<TextureResource>(const std::string& path) {
    return TextureManager::GetInstance()->Load(path);
}

void AssetManager::ClearUnused() {
    ModelManager::GetInstance()->ClearUnused();
    TextureManager::GetInstance()->ClearUnused();
}

} // namespace AbsoluteEngine
