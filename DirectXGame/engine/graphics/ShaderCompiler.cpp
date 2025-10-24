#include "ShaderCompiler.h"
#include "TextureUtils.h" // ConvertString, ログ用  :contentReference[oaicite:2]{index=2}
#include <cassert>
#include <format>

using Microsoft::WRL::ComPtr;

void ShaderCompiler::Initialize(IDxcUtils *utils, IDxcCompiler3 *compiler,
                                IDxcIncludeHandler *include) {
  utils_ = utils;
  compiler_ = compiler;
  include_ = include;
}

ComPtr<IDxcBlob>
ShaderCompiler::Compile(const std::wstring &path, const wchar_t *profile,
                        const std::vector<LPCWSTR> &extraArgs) {
  // ソース読み込み
  ComPtr<IDxcBlobEncoding> source;
  HRESULT hr = utils_->LoadFile(path.c_str(), nullptr, &source);
  assert(SUCCEEDED(hr) && "DXC: LoadFile failed");

  DxcBuffer buf{};
  buf.Ptr = source->GetBufferPointer();
  buf.Size = source->GetBufferSize();
  buf.Encoding = DXC_CP_UTF8;

  // 引数
  std::vector<LPCWSTR> args{
      path.c_str(), L"-E",
      L"main",      L"-T",
      profile,
#ifdef _DEBUG
      L"-Zi",       L"-Qembed_debug",
      L"-Od",
#else
      L"-O3",
#endif
      L"-Zpr" // 行優先
  };
  args.insert(args.end(), extraArgs.begin(), extraArgs.end());

  // コンパイル
  ComPtr<IDxcResult> result;
  hr = compiler_->Compile(&buf, args.data(), (UINT)args.size(), include_.Get(),
                          IID_PPV_ARGS(&result));
  assert(SUCCEEDED(hr) && "DXC: Compile failed");

  // エラー出力
  ComPtr<IDxcBlobUtf8> errors;
  result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
  if (errors && errors->GetStringLength() > 0) {
    OutputDebugStringA(errors->GetStringPointer());
    // 必要なら assert(false);
  }

  // オブジェクト取得
  ComPtr<IDxcBlob> blob;
  hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr);
  assert(SUCCEEDED(hr) && "DXC: Get DXIL failed");
  return blob;
}
