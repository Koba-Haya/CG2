#include "TextureUtils.h" // ConvertString が必要なら

#include <Windows.h>
#include <cassert>
#include <format>
// windows.hより後にdxcapi.hをincludeすること
#include "ShaderCompilerUtils.h"

void Log(const std::string &message) { OutputDebugStringA(message.c_str()); }

IDxcBlob *CompileShader(const std::wstring &filePath, const wchar_t *profile,
                        IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
                        IDxcIncludeHandler *includeHandler) {
  Log(ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n",
                                filePath, profile)));

  IDxcBlobEncoding *shaderSource = nullptr;
  HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
  assert(SUCCEEDED(hr));

  DxcBuffer shaderSourceBuffer;
  shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
  shaderSourceBuffer.Size = shaderSource->GetBufferSize();
  shaderSourceBuffer.Encoding = DXC_CP_UTF8;

  LPCWSTR arguments[] = {
      filePath.c_str(), L"-E",  L"main", L"-T", profile, L"-Zi",
      L"-Qembed_debug", L"-Od", L"-Zpr",
  };

  IDxcResult *shaderResult = nullptr;
  hr = dxcCompiler->Compile(&shaderSourceBuffer, arguments, _countof(arguments),
                            includeHandler, IID_PPV_ARGS(&shaderResult));
  if (FAILED(hr)) {
    OutputDebugStringA("Shader compile failed.\n");
    return nullptr;
  }

  IDxcBlobUtf8 *shaderError = nullptr;
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
  if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Log(shaderError->GetStringPointer());
    assert(false);
  }

  IDxcBlob *shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
                               nullptr);
  assert(SUCCEEDED(hr));

  Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n",
                                filePath, profile)));
  shaderSource->Release();
  shaderResult->Release();
  return shaderBlob;
}