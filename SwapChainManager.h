/*   “スワップチェーン + Present + バックバッファのインデックス”を担当   */


#pragma once
#include <Windows.h>
#include <cstdint>
#include <dxgi1_6.h>
#include <wrl/client.h>

class DirectXDevice;
class CommandSystem;

class SwapChainManager {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  static constexpr uint32_t kFrameCount = 2;

  SwapChainManager() = default;
  ~SwapChainManager() = default;

  bool Initialize(DirectXDevice &dev, CommandSystem &cmd, HWND hwnd,
                  uint32_t width,
                  uint32_t height);
  void Resize(DirectXDevice &dev, CommandSystem &cmd, uint32_t width,
              uint32_t height);

  // Present と現在のインデックス
  void Present(uint32_t syncInterval = 1);
  uint32_t CurrentBackBufferIndex() const {
    return swapChain_->GetCurrentBackBufferIndex();
  }

  IDXGISwapChain4 *Get() const { return swapChain_.Get(); }

private:
  Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
  DXGI_FORMAT format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
};
