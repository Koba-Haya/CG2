#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class DirectXCommon;
class TextureResource;

class TextureManager {
public:
    static TextureManager* GetInstance() {
        static TextureManager inst;
        return &inst;
    }

    void Initialize(DirectXCommon* dx) { dx_ = dx; }

    std::shared_ptr<TextureResource> Load(const std::string& path);
    void ClearUnused();

private:
    TextureManager() = default;

    DirectXCommon* dx_ = nullptr;
    std::unordered_map<std::string, std::weak_ptr<TextureResource>> cache_;
};
