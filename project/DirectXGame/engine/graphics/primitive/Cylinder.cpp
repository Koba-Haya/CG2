#include "Cylinder.h"
#include "Method.h"
#include <cmath>
#include <stdexcept>

void Cylinder::Initialize(ID3D12Device* device, const Params& params) {
    params_ = params;
    GenerateVertices_();
    CreateBuffers_(device);
}

void Cylinder::Update(ID3D12Device* device, const Params& params) {
    params_ = params;
    GenerateVertices_();

    UINT requiredVBSize = static_cast<UINT>(sizeof(Vertex) * vertices_.size());
    UINT requiredIBSize = static_cast<UINT>(sizeof(uint32_t) * indices_.size());

    bool recreateVB = !vertexBuffer_ || vertexBuffer_->GetDesc().Width < requiredVBSize;
    bool recreateIB = !indexBuffer_ || indexBuffer_->GetDesc().Width < requiredIBSize;

    // 頂点バッファの更新
    if (recreateVB) {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = requiredVBSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&vertexBuffer_));

        vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
        vbView_.SizeInBytes = requiredVBSize;
        vbView_.StrideInBytes = sizeof(Vertex);
    }

    if (vertexBuffer_ && requiredVBSize > 0) {
        void* mappedData = nullptr;
        vertexBuffer_->Map(0, nullptr, &mappedData);
        std::memcpy(mappedData, vertices_.data(), requiredVBSize);
        vertexBuffer_->Unmap(0, nullptr);
    }

    // インデックスバッファの更新
    if (recreateIB) {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = requiredIBSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&indexBuffer_));

        ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
        ibView_.SizeInBytes = requiredIBSize;
        ibView_.Format = DXGI_FORMAT_R32_UINT;
    }

    if (indexBuffer_ && requiredIBSize > 0) {
        void* mappedData = nullptr;
        indexBuffer_->Map(0, nullptr, &mappedData);
        std::memcpy(mappedData, indices_.data(), requiredIBSize);
        indexBuffer_->Unmap(0, nullptr);
    }
}

void Cylinder::Draw(ID3D12GraphicsCommandList* cmdList) {
    if (!vertexBuffer_ || !indexBuffer_) return;

    cmdList->IASetVertexBuffers(0, 1, &vbView_);
    cmdList->IASetIndexBuffer(&ibView_);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

void Cylinder::GenerateVertices_() {
    vertices_.clear();
    indices_.clear();

    const float angleRange = params_.endAngle - params_.startAngle;
    const float radianPerDivide = angleRange / static_cast<float>(params_.divide);

    vertices_.reserve(params_.divide * 4);
    indices_.reserve(params_.divide * 6);

    for (uint32_t i = 0; i < params_.divide; ++i) {
        float angle = params_.startAngle + static_cast<float>(i) * radianPerDivide;
        float angleNext = params_.startAngle + static_cast<float>(i + 1) * radianPerDivide;

        float sin = std::sin(angle);
        float cos = std::cos(angle);
        float sinNext = std::sin(angleNext);
        float cosNext = std::cos(angleNext);

        float u = static_cast<float>(i) / static_cast<float>(params_.divide);
        float uNext = static_cast<float>(i + 1) / static_cast<float>(params_.divide);

        // uv flip
        float vTop = params_.flipV ? 1.0f : 0.0f;
        float vBottom = params_.flipV ? 0.0f : 1.0f;

        Vertex vTopLeft, vTopRight, vBottomLeft, vBottomRight;

        if (params_.uvVertical) {
            vTopLeft.texcoord = { vTop, u };
            vTopRight.texcoord = { vTop, uNext };
            vBottomLeft.texcoord = { vBottom, u };
            vBottomRight.texcoord = { vBottom, uNext };
        } else {
            vTopLeft.texcoord = { u, vTop };
            vTopRight.texcoord = { uNext, vTop };
            vBottomLeft.texcoord = { u, vBottom };
            vBottomRight.texcoord = { uNext, vBottom };
        }

        // X, Z 平面での計算（Yが高さ）
        vTopLeft.position = { -sin * params_.topRadiusX, params_.height, cos * params_.topRadiusZ, 1.0f };
        vTopLeft.color = params_.colorTop;

        vTopRight.position = { -sinNext * params_.topRadiusX, params_.height, cosNext * params_.topRadiusZ, 1.0f };
        vTopRight.color = params_.colorTop;

        vBottomLeft.position = { -sin * params_.bottomRadiusX, 0.0f, cos * params_.bottomRadiusZ, 1.0f };
        vBottomLeft.color = params_.colorBottom;

        vBottomRight.position = { -sinNext * params_.bottomRadiusX, 0.0f, cosNext * params_.bottomRadiusZ, 1.0f };
        vBottomRight.color = params_.colorBottom;

        vertices_.push_back(vTopLeft);
        vertices_.push_back(vTopRight);
        vertices_.push_back(vBottomLeft);
        vertices_.push_back(vBottomRight);

        // インデックス (時計回り)
        uint32_t offset = i * 4;
        indices_.push_back(offset + 0);
        indices_.push_back(offset + 1);
        indices_.push_back(offset + 2);

        indices_.push_back(offset + 2);
        indices_.push_back(offset + 1);
        indices_.push_back(offset + 3);
    }

    indexCount_ = static_cast<UINT>(indices_.size());
}

void Cylinder::CreateBuffers_(ID3D12Device* device) {
    if (vertices_.empty()) return;

    // 頂点バッファ
    {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = sizeof(Vertex) * vertices_.size();
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&vertexBuffer_));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create Cylinder VertexBuffer");
        }

        void* mappedData = nullptr;
        vertexBuffer_->Map(0, nullptr, &mappedData);
        std::memcpy(mappedData, vertices_.data(), sizeof(Vertex) * vertices_.size());
        vertexBuffer_->Unmap(0, nullptr);

        vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
        vbView_.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertices_.size());
        vbView_.StrideInBytes = sizeof(Vertex);
    }

    // インデックスバッファ
    {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = sizeof(uint32_t) * indices_.size();
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&indexBuffer_));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create Cylinder IndexBuffer");
        }

        void* mappedData = nullptr;
        indexBuffer_->Map(0, nullptr, &mappedData);
        std::memcpy(mappedData, indices_.data(), sizeof(uint32_t) * indices_.size());
        indexBuffer_->Unmap(0, nullptr);

        ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
        ibView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indices_.size());
        ibView_.Format = DXGI_FORMAT_R32_UINT;
    }

    // 定数バッファ生成用ヘルパーラムダ
    auto CreateCB = [&](UINT size, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer) {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = (size + 255) & ~255; // 256バイトアライメント
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&buffer));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create Cylinder ConstantBuffer");
        }
    };

    CreateCB(sizeof(TransformCB), transformCB_);
    CreateCB(sizeof(MaterialCB), materialCB_);

    // 初期値設定
    SetMaterial({1.0f, 1.0f, 1.0f, 1.0f}, MakeIdentity4x4());
}

void Cylinder::SetTransform(const Matrix4x4& world, const Matrix4x4& view, const Matrix4x4& proj) {
    if (!transformCB_) return;
    void* mapped = nullptr;
    transformCB_->Map(0, nullptr, &mapped);
    if (mapped) {
        TransformCB* cb = static_cast<TransformCB*>(mapped);
        cb->World = world;
        cb->WVP = Multiply(Multiply(world, view), proj);
        transformCB_->Unmap(0, nullptr);
    }
}

void Cylinder::SetMaterial(const Vector4& color, const Matrix4x4& uvTransform) {
    if (!materialCB_) return;
    void* mapped = nullptr;
    materialCB_->Map(0, nullptr, &mapped);
    if (mapped) {
        MaterialCB* cb = static_cast<MaterialCB*>(mapped);
        cb->color = color;
        cb->enableLighting = 0; // ライティングなし
        cb->alphaReference = params_.alphaReference;
        cb->pad0 = 0.0f;
        cb->uvTransform = uvTransform;
        materialCB_->Unmap(0, nullptr);
    }
}
