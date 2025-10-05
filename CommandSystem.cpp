// CommandSystem.cpp
#include "CommandSystem.h"
#include "DirectXDevice.h"
#include <cassert>

CommandSystem::~CommandSystem() { Finalize(); }

bool CommandSystem::Initialize(DirectXDevice &dev) {
  // 1) キュー
  D3D12_COMMAND_QUEUE_DESC qdesc{};
  qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  qdesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  if (FAILED(dev.Get()->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&queue_)))) {
    return false;
  }

  // 2) フェンス＆イベント
  if (FAILED(dev.Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                    IID_PPV_ARGS(&fence_)))) {
    return false;
  }
  fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  // 3) フレーム分のAllocator
  for (uint32_t i = 0; i < kFrameCount; ++i) {
    if (FAILED(dev.Get()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&frames_[i].allocator)))) {
      return false;
    }
    frames_[i].fenceValue = 0; // まだCPU側未使用
  }

  // 4) コマンドリスト（最初はClose）
  if (FAILED(dev.Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          frames_[0].allocator.Get(), nullptr,
                                          IID_PPV_ARGS(&list_)))) {
    return false;
  }
  list_->Close();
  return true;
}

void CommandSystem::Finalize() {
  if (queue_ && fence_) {
    WaitForIdle();
  }
  if (fenceEvent_) {
    CloseHandle(fenceEvent_);
    fenceEvent_ = nullptr;
  }
  // ComPtrは自動解放
}

ID3D12GraphicsCommandList *CommandSystem::BeginFrame(uint32_t frameIndex) {
  // この frameIndex のフレームが前回 GPU
  // に投げられていたら、それが終わるまで待つ
  const uint64_t submitted = frames_[frameIndex].fenceValue;
  if (submitted != 0 && fence_->GetCompletedValue() < submitted) {
    fence_->SetEventOnCompletion(submitted, fenceEvent_);
    WaitForSingleObject(fenceEvent_, INFINITE);
  }

  // 安全に Reset
  frames_[frameIndex].allocator->Reset();
  list_->Reset(frames_[frameIndex].allocator.Get(), nullptr);
  return list_.Get();
}

void CommandSystem::ExecuteAndSignal(uint32_t frameIndex) {
  // リストを閉じて送出
  list_->Close();
  ID3D12CommandList *lists[] = {list_.Get()};
  queue_->ExecuteCommandLists(1, lists);

  // Signal。今回のフレームの完了値として記録
  const uint64_t value = fenceValue_++;
  queue_->Signal(fence_.Get(), value);
  frames_[frameIndex].fenceValue = value;
}

void CommandSystem::WaitForFence(uint64_t value) {
  if (value == 0)
    return;
  if (fence_->GetCompletedValue() < value) {
    fence_->SetEventOnCompletion(value, fenceEvent_);
    WaitForSingleObject(fenceEvent_, INFINITE);
  }
}

void CommandSystem::WaitForIdle() {
  // 現時点までのキューを Signal し、その値を待つ
  const uint64_t value = fenceValue_++;
  queue_->Signal(fence_.Get(), value);
  WaitForFence(value);
}
