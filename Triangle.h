#pragma once
#include "externals/DirectXTex/d3dx12.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#pragma comment(lib, "d3dcompiler.lib")

/**
 * いちばん小さい三角形描画クラス。
 * - Initialize(device*): ルートシグネチャ/PSO/頂点VB を構築
 * - Draw(cmdList): RS/PSO/VB をセットして DrawInstanced(3)
 */
class Triangle {
public:
  Triangle() = default;

  // ← ここを ID3D12Device* にする（DirectXDevice& ではなく）
  bool Initialize(ID3D12Device *device) {
    device_ = device;
    return CreateRootSignature_() && CreateShadersAndPSO_() &&
           CreateVertexBuffer_();
  }

  void Draw(ID3D12GraphicsCommandList *cl) {
    cl->SetPipelineState(pso_.Get());
    cl->SetGraphicsRootSignature(rootSig_.Get());
    cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cl->IASetVertexBuffers(0, 1, &vbView_);
    cl->DrawInstanced(3, 1, 0, 0);
  }

  void Release() {
    vb_.Reset();
    pso_.Reset();
    rootSig_.Reset();
    device_ = nullptr;
  }

private:
  bool CreateRootSignature_() {
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                 D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                 D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                 D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    Microsoft::WRL::ComPtr<ID3DBlob> sig, err;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                           &sig, &err))) {
      return false;
    }
    return SUCCEEDED(device_->CreateRootSignature(0, sig->GetBufferPointer(),
                                                  sig->GetBufferSize(),
                                                  IID_PPV_ARGS(&rootSig_)));
  }

  bool CreateShadersAndPSO_() {
    // クリップ空間にそのまま出す最小VS / 固定色のPS
    static const char *kVS = R"(
            struct VSIn { float3 pos : POSITION; };
            struct VSOut { float4 svpos : SV_POSITION; };
            VSOut main(VSIn i){ VSOut o; o.svpos = float4(i.pos,1); return o; }
        )";
    static const char *kPS = R"(
            float4 main() : SV_TARGET { return float4(0.2, 0.7, 1.0, 1.0); }
        )";
    Microsoft::WRL::ComPtr<ID3DBlob> vs, ps, err;
    if (FAILED(D3DCompile(kVS, strlen(kVS), nullptr, nullptr, nullptr, "main",
                          "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vs,
                          &err)))
      return false;
    if (FAILED(D3DCompile(kPS, strlen(kPS), nullptr, nullptr, nullptr, "main",
                          "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps,
                          &err)))
      return false;

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = rootSig_.Get();
    pso.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    pso.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    pso.InputLayout = {layout, _countof(layout)};
    pso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // あなたのDepthに合わせる
    pso.SampleDesc = {1, 0};

    return SUCCEEDED(
        device_->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pso_)));
  }

  bool CreateVertexBuffer_() {
    struct V {
      float x, y, z;
    };
    const V verts[3] = {
        {0.0f, 0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {-0.5f, -0.5f, 0.0f}};
    const UINT size = sizeof(verts);

    auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    if (FAILED(device_->CreateCommittedResource(
            &heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vb_)))) {
      return false;
    }
    void *p = nullptr;
    vb_->Map(0, nullptr, &p);
    memcpy(p, verts, size);
    vb_->Unmap(0, nullptr);

    vbView_.BufferLocation = vb_->GetGPUVirtualAddress();
    vbView_.StrideInBytes = sizeof(V);
    vbView_.SizeInBytes = size;
    return true;
  }

private:
  ID3D12Device *device_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
  Microsoft::WRL::ComPtr<ID3D12Resource> vb_;
  D3D12_VERTEX_BUFFER_VIEW vbView_{};
};
