#pragma once

#include <string>
#include <memory>
#include <type_traits>

namespace AbsoluteEngine {

class ModelResource;
class TextureResource;

class AssetManager {
private:
    AssetManager() = default;
    ~AssetManager() = default;

public:
    static AssetManager* GetInstance() {
        static AssetManager instance;
        return &instance;
    }

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // 汎用的なロード関数
    template <typename T>
    std::shared_ptr<T> Load(const std::string& path);

    // 未使用リソースの解放
    void ClearUnused();
};

} // namespace AbsoluteEngine
