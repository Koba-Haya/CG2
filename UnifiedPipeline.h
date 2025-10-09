#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <vector>
#include <wrl.h>

// DXC 前方宣言（main.cpp の CompileShader を使う）
struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct IDxcBlob;
class ShaderCompiler;

struct PipelineDesc {
  // 入力レイアウト
  std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;

  // シェーダのパス
  std::wstring vsPath;
  std::wstring psPath;
  const wchar_t *vsProfile = L"vs_6_0";
  const wchar_t *psProfile = L"ps_6_0";

  // ルートパラメータの有無
  bool usePSMaterial_b0 = true;          // PS: b0
  bool useVSTransform_b0 = true;         // VS: b0
  bool usePSTextureTable_t0 = true;      // PS: t0 (SRVテーブル)
  bool usePSDirectionalLight_b1 = false; // PS: b1（Spriteは通常不要）

  // ラスタ/ブレンド/深度
  bool enableDepth = true;
  bool alphaBlend = false;
  D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;

  // 出力フォーマット
  DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};

class UnifiedPipeline {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  bool Initialize(ID3D12Device *device, ShaderCompiler *shaderCompiler,
                  const PipelineDesc &desc);

  ID3D12RootSignature *GetRootSignature() const { return rootSignature_.Get(); }
  ID3D12PipelineState *GetPipelineState() const { return pso_.Get(); }

  // お手軽プリセット
  static PipelineDesc MakeObject3DDesc(); // 3D（ライティングあり）
  static PipelineDesc
  MakeSpriteDesc(); // 2D（アルファブレンド、ライティングなし）

private:
  ComPtr<ID3D12RootSignature> rootSignature_;
  ComPtr<ID3D12PipelineState> pso_;

  ShaderCompiler *shaderCompiler_ = nullptr;
};
