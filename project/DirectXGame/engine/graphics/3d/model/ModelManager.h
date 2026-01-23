#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class DirectXCommon;
class ModelResource;

class ModelManager {
public:
  static ModelManager *GetInstance() {
    static ModelManager inst;
    return &inst;
  }

  void Initialize(DirectXCommon *dx) { dx_ = dx; }

  // 統一本命: フルパス1本
  std::shared_ptr<ModelResource> Load(const std::string &path);

  // 互換ラッパ
  std::shared_ptr<ModelResource>
  LoadModelResource(const std::string &directoryPath,
                    const std::string &filename) {
    return Load(directoryPath + "/" + filename);
  }

  void ClearUnused();

private:
  ModelManager() = default;

  DirectXCommon *dx_ = nullptr;

  std::unordered_map<std::string, std::weak_ptr<ModelResource>> cache_;
};
