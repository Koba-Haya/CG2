#pragma once
#include "externals/DirectXTex/DirectXTex.h"
#include <string>
#include <wrl.h>

std::wstring ConvertString(const std::string &str);

std::string ConvertString(const std::wstring &str);

DirectX::ScratchImage LoadTexture(const std::string &filePath);
