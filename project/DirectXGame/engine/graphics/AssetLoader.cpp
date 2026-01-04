#include "AssetLoader.h"

const ModelData& AssetLoader::LoadObj(const std::string& directoryPath,
    const std::string& filename) {
    const std::string key = MakeKey_(directoryPath, filename);

    // 既にあるならそれを返す
    if (auto it = objCache_.find(key); it != objCache_.end()) {
        return it->second;
    }

    // 無いなら読む（ModelUtils が mtllib も処理して textureFilePath も埋める）
    ModelData data = LoadObjFile(directoryPath, filename);

    auto [it, inserted] = objCache_.emplace(key, std::move(data));
    (void)inserted;
    return it->second;
}

void AssetLoader::Clear() { objCache_.clear(); }
