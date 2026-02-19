#pragma once

#include <Windows.h>
#include <dxcapi.h>

#include <string>

// CompileShader は生ポインタを返す（呼び出し側が Release する想定）
// 呼び出し側が ComPtr で受ける場合は Attach する。
IDxcBlob *CompileShader(const std::wstring &filePath, const wchar_t *profile,
                        IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
                        IDxcIncludeHandler *includeHandler);