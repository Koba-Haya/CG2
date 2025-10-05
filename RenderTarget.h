/*   “バックバッファからRTV作成 + クリア + 遷移 + ビューポート/シザー”を担当 */

#pragma once
#include "externals/DirectXTex/d3dx12.h"
#include <Windows.h>
#include <array>
#include <cstdint>
#include <wrl/client.h>

class DirectXDevice;
class SwapChainManager;
class CommandSystem;

class RenderTarget {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  static constexpr uint32_t kFrameCount = 2;

  RenderTarget() = default;
  ~RenderTarget() = default;

  bool Initialize(DirectXDevice &dev, SwapChainManager &swap, uint32_t width,
                  uint32_t height);
  void Release();

  // 毎フレームの開始：RTV 遷移＆クリア＆VP/Scissor 設定
  void BeginFrame(DirectXDevice& dev, CommandSystem &cmd, SwapChainManager &swap,
                  const FLOAT clearColor[4],
                  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);

  // 毎フレームの終了：Present 用に STATE_PRESENT に戻す
  void EndFrame(DirectXDevice &dev, CommandSystem &cmd, SwapChainManager &swap);

private:
  // バックバッファリソースとRTVヒープ
  std::array<ComPtr<ID3D12Resource>, kFrameCount>
      backbuffers_{};
  ComPtr<ID3D12DescriptorHeap> rtvHeap_;

  // RTV1枚あたりのインクリメント
  UINT rtvIncSize_ = 0;

  // ビューポート/シザー
  D3D12_VIEWPORT viewport_{};
  D3D12_RECT scissor_{};
};
