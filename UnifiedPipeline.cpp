#include "UnifiedPipeline.h"
#include <cassert>
#include <d3d12.h>
#include <d3dcompiler.h> // D3D12SerializeRootSignature

// main.cpp の関数を参照
IDxcBlob *CompileShader(const std::wstring &filePath, const wchar_t *profile,
                        IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
                        IDxcIncludeHandler *includeHandler);

static D3D12_BLEND_DESC MakeBlendDesc(bool alphaBlend) {
  D3D12_BLEND_DESC b{};
  b.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  if (alphaBlend) {
    b.RenderTarget[0].BlendEnable = TRUE;
    b.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    b.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    b.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    b.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    b.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    b.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  }
  return b;
}

bool UnifiedPipeline::Initialize(ID3D12Device *device, IDxcUtils *dxcUtils,
                                 IDxcCompiler3 *dxcCompiler,
                                 IDxcIncludeHandler *includeHandler,
                                 const PipelineDesc &desc) {
  HRESULT hr = S_OK;

  // --- DescriptorRange (SRV t0) ---
  D3D12_DESCRIPTOR_RANGE srvRange{};
  srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  srvRange.BaseShaderRegister = 0;
  srvRange.NumDescriptors = 1;
  srvRange.OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // --- Root Parameters（フラグに応じて詰める） ---
  D3D12_ROOT_PARAMETER params[4]{};
  UINT numParams = 0;

  if (desc.usePSMaterial_b0) {
    auto &p = params[numParams++];
    p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    p.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    p.Descriptor.ShaderRegister = 0; // b0
  }
  if (desc.useVSTransform_b0) {
    auto &p = params[numParams++];
    p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    p.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    p.Descriptor.ShaderRegister = 0; // b0
  }
  if (desc.usePSTextureTable_t0) {
    auto &p = params[numParams++];
    p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    p.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    p.DescriptorTable.pDescriptorRanges = &srvRange;
    p.DescriptorTable.NumDescriptorRanges = 1; // t0
  }
  if (desc.usePSDirectionalLight_b1) {
    auto &p = params[numParams++];
    p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    p.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    p.Descriptor.ShaderRegister = 1; // b1
  }

  D3D12_STATIC_SAMPLER_DESC samp{};
  samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  samp.AddressU = samp.AddressV = samp.AddressW =
      D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  samp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  samp.MaxLOD = D3D12_FLOAT32_MAX;
  samp.ShaderRegister = 0;
  samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  D3D12_ROOT_SIGNATURE_DESC rs{};
  rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  rs.pParameters = params;
  rs.NumParameters = numParams;
  rs.pStaticSamplers = &samp;
  rs.NumStaticSamplers = 1;

  Microsoft::WRL::ComPtr<ID3DBlob> sig, err;
  hr = D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1,
                                   sig.GetAddressOf(), err.GetAddressOf());
  if (FAILED(hr)) {
    if (err)
      OutputDebugStringA((const char *)err->GetBufferPointer());
    assert(false);
    return false;
  }
  OutputDebugStringW(
      (L"CreatePSO: " + desc.vsPath + L" / " + desc.psPath + L"\n").c_str());
  hr = device->CreateRootSignature(0, sig->GetBufferPointer(),
                                   sig->GetBufferSize(),
                                   IID_PPV_ARGS(&rootSignature_));
  if (FAILED(hr))
    return false;

  // --- InputLayout ---
  D3D12_INPUT_LAYOUT_DESC layout{};
  layout.pInputElementDescs = desc.inputElements.data();
  layout.NumElements = (UINT)desc.inputElements.size();

  // --- Shaders ---
  IDxcBlob *vs = CompileShader(desc.vsPath, desc.vsProfile, dxcUtils,
                               dxcCompiler, includeHandler);
  IDxcBlob *ps = CompileShader(desc.psPath, desc.psProfile, dxcUtils,
                               dxcCompiler, includeHandler);

  // --- Raster / Blend / Depth ---
  D3D12_RASTERIZER_DESC rast{};
  rast.CullMode = desc.cullMode;
  rast.FillMode = D3D12_FILL_MODE_SOLID;

  D3D12_BLEND_DESC blend = MakeBlendDesc(desc.alphaBlend);

  D3D12_DEPTH_STENCIL_DESC depth{};
  depth.DepthEnable = desc.enableDepth ? TRUE : FALSE;
  depth.DepthWriteMask = desc.enableDepth ? D3D12_DEPTH_WRITE_MASK_ALL
                                          : D3D12_DEPTH_WRITE_MASK_ZERO;
  depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  // --- PSO ---
  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
  pso.pRootSignature = rootSignature_.Get();
  pso.InputLayout = layout;
  pso.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
  pso.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
  pso.BlendState = blend;
  pso.RasterizerState = rast;
  pso.NumRenderTargets = 1;
  pso.RTVFormats[0] = desc.rtvFormat;
  pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso.SampleDesc.Count = 1;
  pso.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
  pso.DepthStencilState = depth;
  pso.DSVFormat = desc.dsvFormat;

  pso.InputLayout.pInputElementDescs = desc.inputElements.data();
  pso.InputLayout.NumElements = (UINT)desc.inputElements.size();

  hr = device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pso_));
  return SUCCEEDED(hr);
}

// ===== プリセット =====
PipelineDesc UnifiedPipeline::MakeObject3DDesc() {
  PipelineDesc d{};
  // POSITION(float4) / TEXCOORD(float2) / NORMAL(float3)  ← 既存 Object3D
  // と合せる
  d.inputElements = {
      D3D12_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                               D3D12_APPEND_ALIGNED_ELEMENT,
                               D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      D3D12_INPUT_ELEMENT_DESC{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                               D3D12_APPEND_ALIGNED_ELEMENT,
                               D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      D3D12_INPUT_ELEMENT_DESC{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                               D3D12_APPEND_ALIGNED_ELEMENT,
                               D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
  d.vsPath = L"Object3D.VS.hlsl";
  d.psPath = L"Object3D.PS.hlsl";
  d.usePSMaterial_b0 = true;
  d.useVSTransform_b0 = true;
  d.usePSTextureTable_t0 = true;
  d.usePSDirectionalLight_b1 = true; // ← ライティングあり
  d.enableDepth = true;
  d.alphaBlend = false;
  d.cullMode = D3D12_CULL_MODE_BACK;
  return d;
}

PipelineDesc UnifiedPipeline::MakeSpriteDesc() {
  PipelineDesc d{};
  // Sprite想定：POSITION(float3) / TEXCOORD(float2)
  d.inputElements = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };

  // ここはあなたのスプライト用 HLSL に合わせて変更
  d.vsPath = L"Sprite.VS.hlsl";
  d.psPath = L"Sprite.PS.hlsl";
  d.usePSMaterial_b0 = true;          // 色やUV操作用のCBVがあるなら
  d.useVSTransform_b0 = true;         // 2D行列でもVS:b0でOK
  d.usePSTextureTable_t0 = true;      // テクスチャ読む
  d.usePSDirectionalLight_b1 = false; // ライティング不要
  d.enableDepth = false;              // 2Dなら基本オフ
  d.alphaBlend = true;                // 透過前提
  d.cullMode = D3D12_CULL_MODE_NONE;
  return d;
}

// PipelineDesc UnifiedPipeline::MakeSpriteDesc() {
//   PipelineDesc d{};
//   // 3Dと同じ：POSITION(float4) / TEXCOORD(float2) / NORMAL(float3)
//   d.inputElements = {
//       {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
//        D3D12_APPEND_ALIGNED_ELEMENT,
//        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//       {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
//       D3D12_APPEND_ALIGNED_ELEMENT,
//        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//       {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
//        D3D12_APPEND_ALIGNED_ELEMENT,
//        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//   };
//
//   // Object3D の HLSL をそのまま使う
//   d.vsPath = L"Object3D.VS.hlsl";
//   d.psPath = L"Object3D.PS.hlsl";
//
//   // ルートパラメータも 3D と同じ並びにする（PS:b0, VS:b0, PS:t0, PS:b1）
//   d.usePSMaterial_b0 = true;
//   d.useVSTransform_b0 = true;
//   d.usePSTextureTable_t0 = true;
//   d.usePSDirectionalLight_b1 = true; // ※必須（cbuffer 宣言があるため）
//
//   // 2D らしい設定
//   d.enableDepth = false;
//   d.alphaBlend = true;
//   d.cullMode = D3D12_CULL_MODE_NONE;
//   return d;
// }