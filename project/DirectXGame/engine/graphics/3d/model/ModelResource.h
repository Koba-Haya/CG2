#pragma once

#include <cstdint>
#include <memory>

struct ModelData;
class TextureResource;
class DirectXCommon;

class ModelResource {
public:
  struct CreateInfo {
    DirectXCommon *dx = nullptr;
    std::shared_ptr<const ModelData> modelData;
    std::shared_ptr<TextureResource> texture;
  };

  ModelResource();
  ~ModelResource(); // 実装は .cpp へ

  ModelResource(const ModelResource &) = delete;
  ModelResource &operator=(const ModelResource &) = delete;

  bool Initialize(const CreateInfo &ci);

  const ModelData* GetModelData() const;

  // Renderer が使用するアクセサ
  unsigned long long GetVBVAddress() const;
  unsigned int GetVBVSize() const;
  unsigned int GetVBVStride() const;
  uint32_t GetVertexCount() const;

  unsigned long long GetIBVAddress() const;
  unsigned int GetIBVSize() const;
  uint32_t GetIndexCount() const;

  unsigned long long GetTextureHandleGPUAsUInt64() const;
  
  bool HasBones() const;
  unsigned long long GetBoneVBVAddress() const;
  unsigned int GetBoneVBVSize() const;
  unsigned int GetBoneVBVStride() const;

private:
  struct Impl;
  std::unique_ptr<Impl> pImpl_;
};