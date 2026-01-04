#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class DirectXCommon;
class ModelResource;
class SrvAllocator;

class ModelManager {
public:
    static ModelManager* GetInstance() {
        static ModelManager inst;
        return &inst;
    }

    void Initialize(DirectXCommon* dx, SrvAllocator* srvAlloc) {
        dx_ = dx;
        srvAlloc_ = srvAlloc;
    }

    // dir + filename に合わせる
    std::shared_ptr<ModelResource> LoadObj(const std::string& directoryPath,
        const std::string& filename);

    void ClearUnused();

private:
    ModelManager() = default;

    std::string MakeKey_(const std::string& dir, const std::string& file) const {
        return dir + "/" + file;
    }

    DirectXCommon* dx_ = nullptr;
    SrvAllocator* srvAlloc_ = nullptr;

    std::unordered_map<std::string, std::weak_ptr<ModelResource>> cache_;
};
