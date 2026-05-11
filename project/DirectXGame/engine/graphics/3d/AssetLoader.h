#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "ModelUtils.h"

class AssetLoader {
public:
  static AssetLoader *GetInstance() {
    static AssetLoader inst;
    return &inst;
  }

  std::shared_ptr<const ModelData> LoadModel(const std::string &directoryPath,
                                             const std::string &filename);

  // フルパス1本版
  std::shared_ptr<const ModelData> LoadModel(const std::string &path);

  void Clear() { cache_.clear(); }

private:
  AssetLoader() = default;

  std::string MakeKey_(const std::string &dir, const std::string &file) const {
    return dir + "/" + file;
  }

  Node ReadNode_(const aiNode *node);
  void BuildSkeleton_(const aiNode *node, Skeleton &skeleton, int32_t parentJointIndex, std::map<uint32_t, int32_t>& meshToJointMap);

  std::unordered_map<std::string, std::shared_ptr<ModelData>> cache_;
};
