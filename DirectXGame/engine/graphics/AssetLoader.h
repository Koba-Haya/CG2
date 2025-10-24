#pragma once
#include "Model.h" // ModelData, MaterialData
#include <string>
#include <unordered_map>

class AssetLoader {
public:
  const ModelData &LoadObj(const std::string &dir, const std::string &file);
  const MaterialData &LoadMtl(const std::string &dir, const std::string &file);

private:
  std::unordered_map<std::string, ModelData> modelCache_;
  std::unordered_map<std::string, MaterialData> materialCache_;

  // 内部実装（実際の読み込み）
  ModelData LoadObjImpl(const std::string &dir, const std::string &file);
  MaterialData LoadMtlImpl(const std::string &dir,
                                  const std::string &file);
};
