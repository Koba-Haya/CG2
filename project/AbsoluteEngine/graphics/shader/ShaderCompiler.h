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
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  void Initialize(IDxcUtils *utils, IDxcCompiler3 *compiler,
                  IDxcIncludeHandler *include);

  ComPtr<IDxcBlob> Compile(const std::wstring &path, const wchar_t *profile,
                           const std::vector<LPCWSTR> &extraArgs = {});

  // 互換用：生ポインタを返す版（UnifiedPipeline から使いやすく）
  IDxcBlob *CompileRaw(const std::wstring &path, const wchar_t *profile,
                       const std::vector<LPCWSTR> &extraArgs = {});

private:
  ComPtr<IDxcUtils> utils_;
  ComPtr<IDxcCompiler3> compiler_;
  ComPtr<IDxcIncludeHandler> include_;
};