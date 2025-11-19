#pragma once
#include <dxcapi.h>
#include <string>

// main.cpp にあった CompileShader と同じシグネチャ
IDxcBlob *CompileShader(const std::wstring &filePath, const wchar_t *profile,
                        IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
                        IDxcIncludeHandler *includeHandler);