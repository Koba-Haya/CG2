#pragma once

#include <Windows.h>
#include <cassert>
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <memory>

#include "DirectXCommon.h"
#include "Matrix.h"
#include "Method.h"
#include "UnifiedPipeline.h"
#include "Vector.h"

class TextureResource;

class Sprite {
public:
    struct CreateInfo {
        DirectXCommon* dx = nullptr;
        UnifiedPipeline* pipeline = nullptr;
        std::string texturePath;
        Vector2 size = { 640, 360 };
        Vector4 color = { 1, 1, 1, 1 };
    };

    bool Initialize(const CreateInfo& info);
    void SetPosition(const Vector3& pos);
    void SetScale(const Vector3& scale);
    void SetRotation(const Vector3& rot);
    void SetUVTransform(const Matrix4x4& uv);
    void SetColor(const Vector4& color);
    void SetPipeline(UnifiedPipeline* pipeline) { pipeline_ = pipeline; }
    void Draw(const Matrix4x4& view, const Matrix4x4& proj);

private:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

private:
    DirectXCommon* dx_ = nullptr;
    UnifiedPipeline* pipeline_ = nullptr;

    std::shared_ptr<TextureResource> texture_;
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

    ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    ComPtr<ID3D12Resource> materialBuffer_;
    ComPtr<ID3D12Resource> transformBuffer_;

    struct SpriteMaterial {
        Vector4 color;
        Matrix4x4 uvTransform;
    };
    struct SpriteTransform {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };

    SpriteMaterial* materialMapped_ = nullptr;
    SpriteTransform* transformMapped_ = nullptr;

    Matrix4x4 worldMatrix_ = MakeIdentity4x4();
    Vector3 position_{ 0, 0, 0 };
    Vector3 scale_{ 1, 1, 1 };
    Vector3 rotation_{ 0, 0, 0 };
    Matrix4x4 uvMatrix_ = MakeIdentity4x4();
    Vector4 color_ = { 1, 1, 1, 1 };
};
