#pragma once
#include <string>
#include <unordered_map>

#include "ModelUtils.h" // ModelData / LoadObjFile

class AssetLoader {
public:
    static AssetLoader* GetInstance() {
        static AssetLoader inst;
        return &inst;
    }

    // dir + filename で読む（例: "resources/sphere", "sphere.obj"）
    const ModelData& LoadObj(const std::string& directoryPath,
        const std::string& filename);

    void Clear();

private:
    AssetLoader() = default;

    std::string MakeKey_(const std::string& dir, const std::string& file) const {
        return dir + "/" + file;
    }

    std::unordered_map<std::string, ModelData> objCache_;
};
