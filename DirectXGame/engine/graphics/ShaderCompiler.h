#pragma once
#include <string>
#include <vector>
#include <wrl.h>

// ※必ず Windows.h を dxcapi.h より前に
#include <Windows.h>
#include <d3d12.h>
#include <dxcapi.h>

class ShaderCompiler {
public:
  // 省略しない ComPtr エイリアス
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  // DirectXCommon が作った DXC を借りる前提
  void Initialize(IDxcUtils *utils, IDxcCompiler3 *compiler,
                  IDxcIncludeHandler *include);

  // 便利関数：HLSL をコンパイルして Blob を返す
  ComPtr<IDxcBlob> Compile(const std::wstring &path, const wchar_t *profile,
                           const std::vector<LPCWSTR> &extraArgs = {});

private:
  ComPtr<IDxcUtils> utils_;
  ComPtr<IDxcCompiler3> compiler_;
  ComPtr<IDxcIncludeHandler> include_;
};
