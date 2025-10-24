#include "TextureUtils.h"
#include <cassert>
#include <filesystem>
#include <Windows.h>

std::wstring ConvertString(const std::string &str) {
  if (str.empty()) {
    return std::wstring();
  }

  auto sizeNeeded =
      MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
                          static_cast<int>(str.size()), NULL, 0);
  if (sizeNeeded == 0) {
    return std::wstring();
  }
  std::wstring result(sizeNeeded, 0);
  MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
                      static_cast<int>(str.size()), &result[0], sizeNeeded);
  return result;
}

std::string ConvertString(const std::wstring &str) {
  if (str.empty()) {
    return std::string();
  }

  auto sizeNeeded =
      WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                          NULL, 0, NULL, NULL);
  if (sizeNeeded == 0) {
    return std::string();
  }
  std::string result(sizeNeeded, 0);
  WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                      result.data(), sizeNeeded, NULL, NULL);
  return result;
}

DirectX::ScratchImage LoadTexture(const std::string &filePath) {
  using namespace std;
  using namespace std::filesystem;
  DirectX::ScratchImage mipImages{};

  // exe と同じフォルダからの絶対パスに直す
  wchar_t exePathW[MAX_PATH]{};
  GetModuleFileNameW(nullptr, exePathW, MAX_PATH);
  path exeDir = path(exePathW).parent_path();

  path candidate =
      exeDir /
      ConvertString(filePath); // e.g. ...\Debug\resources\uvChecker.png
  if (!exists(candidate)) {
    // プロジェクト直下の resources も一応探す
    path alt = exeDir.parent_path().parent_path() /
               ConvertString(filePath); // ..\..\resources\...
    if (exists(alt))
      candidate = alt;
  }

  if (!exists(candidate)) {
    std::string msg = "LoadTexture: file NOT found: " + filePath + "\n";
    OutputDebugStringA(msg.c_str());
    // 失敗時は空の画像を返す（assertで止めたいならここでassertでもOK）
    return mipImages;
  }

  DirectX::ScratchImage image{};
  HRESULT hr = DirectX::LoadFromWICFile(
      candidate.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  if (FAILED(hr)) {
    std::string msg = "LoadTexture: LoadFromWICFile failed: " + filePath + "\n";
    OutputDebugStringA(msg.c_str());
    return mipImages;
  }

  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  if (FAILED(hr)) {
    std::string msg = "LoadTexture: GenerateMipMaps failed: " + filePath + "\n";
    OutputDebugStringA(msg.c_str());
    return mipImages;
  }
  return mipImages;
}
