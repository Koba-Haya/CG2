#include "UnifiedPipeline.h"
#include <cassert>
#include <d3d12.h>
#include <d3dcompiler.h> // D3D12SerializeRootSignature
#include "ShaderCompilerUtils.h"

static D3D12_BLEND_DESC MakeBlendDesc(const PipelineDesc &desc) {
  D3D12_BLEND_DESC b{};
  b.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

  if (!desc.alphaBlend) {
    // ブレンドなし（Opaque）
    return b;
  }

  auto &rt = b.RenderTarget[0];
  rt.BlendEnable = TRUE;

  switch (desc.blendMode) {
  case BlendMode::Alpha: // 通常アルファ
  default:
    rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    break;

  case BlendMode::Add: // 加算：dst + src * a
    rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend = D3D12_BLEND_ONE;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_ONE;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    break;

  case BlendMode::Subtract: // 減算：dst - src * a
    rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend = D3D12_BLEND_ONE;
    rt.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // dst - src
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_ONE;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    break;

  case BlendMode::Multiply: // 乗算：dst * src
    rt.SrcBlend = D3D12_BLEND_ZERO;
    rt.DestBlend = D3D12_BLEND_SRC_COLOR;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ZERO;
    rt.DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    break;

  case BlendMode::Screen: // スクリーンっぽい近似
    // 1 - (1-s) * (1-d) ≒ s + d - s*d
    rt.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
    rt.DestBlend = D3D12_BLEND_ONE;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    break;
  }

  return b;
}

bool UnifiedPipeline::Initialize(ID3D12Device *device, IDxcUtils *dxcUtils,
                                 IDxcCompiler3 *dxcCompiler,
                                 IDxcIncludeHandler *includeHandler,
                                 const PipelineDesc &desc) {
  HRESULT hr = S_OK;

  // SRV range (PS: texture t0)
  D3D12_DESCRIPTOR_RANGE srvRangeTex{};
  srvRangeTex.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  srvRangeTex.BaseShaderRegister = 0;
  srvRangeTex.NumDescriptors = 1;
  srvRangeTex.OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // SRV range (VS: instancing matrices t1)
  D3D12_DESCRIPTOR_RANGE srvRangeInst{};
  srvRangeInst.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  srvRangeInst.BaseShaderRegister = 1; // t1
  srvRangeInst.NumDescriptors = 1;
  srvRangeInst.OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // --- Root Parameters（フラグに応じて詰める） ---
  D3D12_ROOT_PARAMETER params[5]{};
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
    p.DescriptorTable.pDescriptorRanges = &srvRangeTex;
    p.DescriptorTable.NumDescriptorRanges = 1; // t0
  }
  if (desc.useVSInstancingTable_t1) {
    auto &p = params[numParams++];
    p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    p.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    p.DescriptorTable.pDescriptorRanges = &srvRangeInst;
    p.DescriptorTable.NumDescriptorRanges = 1;
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

  D3D12_BLEND_DESC blend = MakeBlendDesc(desc);

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
  d.vsPath = L"resources/shaders/Object3D.VS.hlsl";
  d.psPath = L"resources/shaders/Object3D.PS.hlsl";
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
  d.inputElements = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
  d.vsPath = L"resources/shaders/Sprite.VS.hlsl";
  d.psPath = L"resources/shaders/Sprite.PS.hlsl";
  d.usePSMaterial_b0 = true;
  d.useVSTransform_b0 = true;
  d.usePSTextureTable_t0 = true;
  d.enableDepth = false;
  d.alphaBlend = true;
  d.cullMode = D3D12_CULL_MODE_NONE;

  // ★ Sprite のデフォルトは Alpha ブレンド
  d.blendMode = BlendMode::Alpha;

  return d;
}

PipelineDesc UnifiedPipeline::MakeParticleDesc() {
  PipelineDesc d{};
  d.inputElements = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
  d.vsPath = L"resources/shaders/Particle.VS.hlsl";
  d.psPath = L"resources/shaders/Particle.PS.hlsl";
  d.usePSMaterial_b0 = true;
  d.useVSTransform_b0 = false;      // Instancingで行列はSRV
  d.usePSTextureTable_t0 = true;
  d.useVSInstancingTable_t1 = true; // t1 StructuredBuffer
  d.usePSDirectionalLight_b1 = false;
  d.enableDepth = false;
  d.alphaBlend = true;              // ← 透過フェードのため有効化
  d.blendMode = BlendMode::Alpha;   // ← 通常アルファ
  d.cullMode = D3D12_CULL_MODE_NONE;
  return d;
}