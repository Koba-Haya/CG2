#include "Sprite.h"
#include "TextureManager.h"
#include "TextureResource.h"
#include <cstring>

struct SpriteVertex {
    Vector3 pos;
    Vector2 uv;
};

constexpr UINT Align256_Sprite(UINT n) { return (n + 255) & ~255; }

Microsoft::WRL::ComPtr<ID3D12Resource>
Sprite::CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = static_cast<UINT64>(sizeInBytes);
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> res;
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res));
    assert(SUCCEEDED(hr));
    return res;
}

bool Sprite::Initialize(const CreateInfo& info) {
    assert(info.dx && info.pipeline);
    dx_ = info.dx;
    pipeline_ = info.pipeline;
    color_ = info.color;

    // TextureManager 経由（SRVはTextureResourceが所有して自動Free）
    texture_ = TextureManager::GetInstance()->Load(info.texturePath);
    textureHandle_ = texture_->GetSrvGpu();

    // 頂点
    const float w = info.size.x;
    const float h = info.size.y;

    SpriteVertex vertices[4] = {
      {{0, h, 0}, {0, 1}},
      {{0, 0, 0}, {0, 0}},
      {{w, h, 0}, {1, 1}},
      {{w, 0, 0}, {1, 0}},
    };

    vertexBuffer_ = CreateBufferResource(dx_->GetDevice(), sizeof(vertices));
    void* v = nullptr;
    vertexBuffer_->Map(0, nullptr, &v);
    std::memcpy(v, vertices, sizeof(vertices));

    vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(vertices);
    vbView_.StrideInBytes = sizeof(SpriteVertex);

    uint32_t indices[6] = { 0, 1, 2, 1, 3, 2 };
    indexBuffer_ = CreateBufferResource(dx_->GetDevice(), sizeof(indices));
    void* i = nullptr;
    indexBuffer_->Map(0, nullptr, &i);
    std::memcpy(i, indices, sizeof(indices));

    ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(indices);
    ibView_.Format = DXGI_FORMAT_R32_UINT;

    // CBはMapしっぱなし
    materialBuffer_ = CreateBufferResource(dx_->GetDevice(), Align256_Sprite(sizeof(SpriteMaterial)));
    materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialMapped_));
    materialMapped_->color = color_;
    materialMapped_->uvTransform = uvMatrix_;

    transformBuffer_ = CreateBufferResource(dx_->GetDevice(), Align256_Sprite(sizeof(SpriteTransform)));
    transformBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&transformMapped_));
    transformMapped_->WVP = MakeIdentity4x4();
    transformMapped_->World = worldMatrix_;

    return true;
}

void Sprite::SetPosition(const Vector3& pos) { position_ = pos; }
void Sprite::SetScale(const Vector3& scale) { scale_ = scale; }
void Sprite::SetRotation(const Vector3& rot) { rotation_ = rot; }
void Sprite::SetUVTransform(const Matrix4x4& uv) { uvMatrix_ = uv; }
void Sprite::SetColor(const Vector4& color) { color_ = color; }

void Sprite::Draw(const Matrix4x4& view, const Matrix4x4& proj) {
    worldMatrix_ = MakeAffineMatrix(scale_, rotation_, position_);
    Matrix4x4 wvp = Multiply(worldMatrix_, Multiply(view, proj));

    transformMapped_->WVP = wvp;
    transformMapped_->World = worldMatrix_;

    materialMapped_->color = color_;
    materialMapped_->uvTransform = uvMatrix_;

    ID3D12GraphicsCommandList* cmd = dx_->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { dx_->GetSRVHeap() };
    cmd->SetDescriptorHeaps(1, heaps);

    cmd->SetGraphicsRootSignature(pipeline_->GetRootSignature());
    cmd->SetPipelineState(pipeline_->GetPipelineState());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &vbView_);
    cmd->IASetIndexBuffer(&ibView_);

    cmd->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(1, transformBuffer_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(2, textureHandle_);

    cmd->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
