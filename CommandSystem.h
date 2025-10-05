/*   “キュー / アロケータ / コマンドリスト / フェンス”を担当   */


#pragma once
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

class DirectXDevice;

struct FrameContext {
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
  uint64_t fenceValue = 0; // このフレームをGPUに投げた時のフェンス値
};

class CommandSystem {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  static constexpr uint32_t kFrameCount = 2; // ダブルバッファ想定

  CommandSystem() = default;
  ~CommandSystem();

  bool Initialize(DirectXDevice &dev);
  void Finalize();

  // frameIndex のフレームを開始する。
  // 必要ならそのフレームの完了を待ってから Allocator を Reset する。
  ID3D12GraphicsCommandList *BeginFrame(uint32_t frameIndex);

  // コマンド送出。Signalして、frameIndex に対応する fenceValue を記録。
  void ExecuteAndSignal(uint32_t frameIndex);

  // 任意の値まで待機
  void WaitForFence(uint64_t value);

  // 全GPU作業の完了待ち（終了時に使うと安全）
  void WaitForIdle();

  // アクセサ
  ID3D12CommandQueue *Queue() const { return queue_.Get(); }
  ID3D12GraphicsCommandList *List() const { return list_.Get(); }
  ID3D12Fence *Fence() const { return fence_.Get(); }
  uint64_t CurrentFenceValue() const { return fenceValue_; }
  HANDLE FenceEvent() const { return fenceEvent_; }

private:
  ComPtr<ID3D12CommandQueue> queue_;
  ComPtr<ID3D12GraphicsCommandList> list_;
  ComPtr<ID3D12Fence> fence_;

  FrameContext frames_[kFrameCount]{};

  uint64_t fenceValue_ = 1; // 次に Signal する値
  HANDLE fenceEvent_ = nullptr;
};
