#pragma once
#include "Vector.h"
#include <memory>
#include <string>

struct ID3D12Resource;
class TextureResource;

struct SpriteVertex {
  Vector3 pos;
  Vector2 uv;
};

class SpriteResource {
public:
  SpriteResource();
  ~SpriteResource(); // 実装は .cpp へ

  bool Initialize(const std::string &texturePath);

  unsigned long long GetVBAddress() const;
  unsigned int GetVBSize() const;
  unsigned int GetVBStride() const { return sizeof(SpriteVertex); }

  unsigned long long GetIBAddress() const;
  unsigned int GetIBSize() const;
  unsigned int GetIndexCount() const { return 6; }

  std::shared_ptr<TextureResource> GetTexture() const;

private:
  struct Impl;
  std::unique_ptr<Impl> pImpl_;
};