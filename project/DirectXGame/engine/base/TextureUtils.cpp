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

ComPtr<ID3D12Resource>
CreateTextureResource(const ComPtr<ID3D12Device> &device,
                      const DirectX::TexMetadata &metadata) {
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);
  resourceDesc.Height = UINT(metadata.height);
  resourceDesc.MipLevels = UINT16(metadata.mipLevels);
  resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
  resourceDesc.Format = metadata.format;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

  ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
  assert(SUCCEEDED(hr));
  return resource;
}

void UploadTextureData(const ComPtr<ID3D12Resource> &texture,
                       const DirectX::ScratchImage &mipImages) {
  const DirectX::TexMetadata &metadata = mipImages.GetMetadata();
  for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
    const DirectX::Image *img = mipImages.GetImage(mipLevel, 0, 0);
    HRESULT hr =
        texture->WriteToSubresource(UINT(mipLevel), nullptr, img->pixels,
                                    UINT(img->rowPitch), UINT(img->slicePitch));
    assert(SUCCEEDED(hr));
  }
}