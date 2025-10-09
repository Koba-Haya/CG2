#include "DirectXCommon.h"
#include "ResourceManager.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <cassert>
#include <dxcapi.h>

using Microsoft::WRL::ComPtr;

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

DirectXCommon::DirectXCommon() {}

DirectXCommon::~DirectXCommon() {
  if (fenceEvent_) {
    CloseHandle(fenceEvent_);
    fenceEvent_ = nullptr;
  }
  if (dxc_.includeHandler) {
    dxc_.includeHandler->Release();
    dxc_.includeHandler = nullptr;
  }
  if (dxc_.compiler) {
    dxc_.compiler->Release();
    dxc_.compiler = nullptr;
  }
  if (dxc_.utils) {
    dxc_.utils->Release();
    dxc_.utils = nullptr;
  }

  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  if (ImGui::GetCurrentContext()) {
    ImGui::DestroyContext();
  }
}

void DirectXCommon::Initialize(const InitParams &params) {
  hInstance_ = params.hInstance;
  hwnd_ = params.hwnd;
  clientWidth_ = params.clientWidth;
  clientHeight_ = params.clientHeight;

#ifdef _DEBUG
  ComPtr<ID3D12Debug1> debugController;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();
    debugController->SetEnableGPUBasedValidation(TRUE);
  }
#endif

  CreateDeviceAndFactory_();
  ChooseAdapter_();
  CreateCommandObjects_();
  CreateSwapChain_();
  CreateDescriptorHeaps_();
  CreateRTVs_();
  CreateDepthStencil_();
  CreateFenceAndEvent_();
  SetupViewportAndScissor_();
  InitDXC_();
  InitImGui_();
}

void DirectXCommon::BeginFrame() {
  UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  commandList_->ResourceBarrier(1, &barrier);

  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      ResourceManager::GetCPUDescriptorHandle(dsvDescriptorHeap_.Get(),
                                              descriptorSizeDSV_, 0);
  commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], FALSE,
                                   &dsvHandle);
}

void DirectXCommon::EndFrame() {
  UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

  // Present 遷移
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  commandList_->ResourceBarrier(1, &barrier);

  HRESULT hr = commandList_->Close();
  assert(SUCCEEDED(hr));

  ID3D12CommandList *lists[]{commandList_.Get()};
  commandQueue_->ExecuteCommandLists(1, lists);
  swapChain_->Present(1, 0);

  fenceValue_++;
  commandQueue_->Signal(fence_.Get(), fenceValue_);
  if (fence_->GetCompletedValue() < fenceValue_) {
    fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
    WaitForSingleObject(fenceEvent_, INFINITE);
  }

  hr = commandAllocator_->Reset();
  assert(SUCCEEDED(hr));
  hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
  assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateDeviceAndFactory_() {
  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
  assert(SUCCEEDED(hr));

  // Adapter は ChooseAdapter_() で決定
}

void DirectXCommon::ChooseAdapter_() {
  for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(
                       i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                       IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND;
       ++i) {
    DXGI_ADAPTER_DESC3 desc{};
    HRESULT hr = useAdapter_->GetDesc3(&desc);
    assert(SUCCEEDED(hr));
    if ((desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == 0) {
      break;
    }
    useAdapter_ = nullptr;
  }
  assert(useAdapter_ != nullptr);

  // Device
  D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1,
                             D3D_FEATURE_LEVEL_12_0};
  for (size_t i = 0; i < _countof(levels); ++i) {
    HRESULT hr =
        D3D12CreateDevice(useAdapter_.Get(), levels[i], IID_PPV_ARGS(&device_));
    if (SUCCEEDED(hr)) {
      break;
    }
  }
  assert(device_ != nullptr);

#ifdef _DEBUG
  ComPtr<ID3D12InfoQueue> infoQueue;
  if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    D3D12_MESSAGE_ID denyIds[] = {
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
    D3D12_MESSAGE_SEVERITY severities[]{D3D12_MESSAGE_SEVERITY_INFO};
    D3D12_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = _countof(denyIds);
    filter.DenyList.pIDList = denyIds;
    filter.DenyList.NumSeverities = _countof(severities);
    filter.DenyList.pSeverityList = severities;
    infoQueue->PushStorageFilter(&filter);
  }
#endif
}

void DirectXCommon::CreateCommandObjects_() {
  HRESULT hr = S_OK;

  D3D12_COMMAND_QUEUE_DESC cqDesc{};
  hr = device_->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue_));
  assert(SUCCEEDED(hr));

  hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       IID_PPV_ARGS(&commandAllocator_));
  assert(SUCCEEDED(hr));

  hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  commandAllocator_.Get(), nullptr,
                                  IID_PPV_ARGS(&commandList_));
  assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateSwapChain_() {
  swapChainDesc_ = {};
  swapChainDesc_.Width = clientWidth_;
  swapChainDesc_.Height = clientHeight_;
  swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc_.SampleDesc.Count = 1;
  swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc_.BufferCount = 2;
  swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
      commandQueue_.Get(), hwnd_, &swapChainDesc_, nullptr, nullptr,
      reinterpret_cast<IDXGISwapChain1 **>(swapChain_.GetAddressOf()));
  assert(SUCCEEDED(hr));

  // バックバッファ取得
  hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
  assert(SUCCEEDED(hr));
  hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
  assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateDescriptorHeaps_() {
  rtvDescriptorHeap_ = ResourceManager::CreateDescriptorHeap(
      device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
  srvDescriptorHeap_ = ResourceManager::CreateDescriptorHeap(
      device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
  dsvDescriptorHeap_ = ResourceManager::CreateDescriptorHeap(
      device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

  descriptorSizeRTV_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  descriptorSizeDSV_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void DirectXCommon::CreateRTVs_() {
  rtvDesc_ = {};
  rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

  D3D12_CPU_DESCRIPTOR_HANDLE rtvStart =
      ResourceManager::GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(),
                                              descriptorSizeRTV_, 0);

  rtvHandles_[0] = rtvStart;
  device_->CreateRenderTargetView(swapChainResources_[0].Get(), &rtvDesc_,
                                  rtvHandles_[0]);

  rtvHandles_[1].ptr = rtvHandles_[0].ptr + descriptorSizeRTV_;
  device_->CreateRenderTargetView(swapChainResources_[1].Get(), &rtvDesc_,
                                  rtvHandles_[1]);
}

void DirectXCommon::CreateDepthStencil_() {
  depthStencilResource_ = ResourceManager::CreateDepthStencilTextureResource(
      device_.Get(), clientWidth_, clientHeight_);

  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

  device_->CreateDepthStencilView(
      depthStencilResource_.Get(), &dsvDesc,
      dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::CreateFenceAndEvent_() {
  HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE,
                                    IID_PPV_ARGS(&fence_));
  assert(SUCCEEDED(hr));
  fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
  assert(fenceEvent_ != nullptr);
}

void DirectXCommon::SetupViewportAndScissor_() {
  viewport_ = {};
  viewport_.Width = static_cast<float>(clientWidth_);
  viewport_.Height = static_cast<float>(clientHeight_);
  viewport_.TopLeftX = 0.0f;
  viewport_.TopLeftY = 0.0f;
  viewport_.MinDepth = 0.0f;
  viewport_.MaxDepth = 1.0f;

  scissorRect_ = {};
  scissorRect_.left = 0;
  scissorRect_.top = 0;
  scissorRect_.right = clientWidth_;
  scissorRect_.bottom = clientHeight_;
}

void DirectXCommon::InitDXC_() {
  HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxc_.utils));
  assert(SUCCEEDED(hr));
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_.compiler));
  assert(SUCCEEDED(hr));
  hr = dxc_.utils->CreateDefaultIncludeHandler(&dxc_.includeHandler);
  assert(SUCCEEDED(hr));
}

void DirectXCommon::InitImGui_() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(hwnd_);
  ImGui_ImplDX12_Init(device_.Get(), swapChainDesc_.BufferCount,
                      rtvDesc_.Format, srvDescriptorHeap_.Get(),
                      ResourceManager::GetCPUDescriptorHandle(
                          srvDescriptorHeap_.Get(), descriptorSizeSRV_, 0),
                      ResourceManager::GetGPUDescriptorHandle(
                          srvDescriptorHeap_.Get(), descriptorSizeSRV_, 0));
}

void DirectXCommon::TransitionBackBufferToRenderTarget_() {
  UINT i = swapChain_->GetCurrentBackBufferIndex();
  D3D12_RESOURCE_BARRIER b{};
  b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  b.Transition.pResource = swapChainResources_[i].Get();
  b.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  commandList_->ResourceBarrier(1, &b);
}

void DirectXCommon::TransitionBackBufferToPresent_() {
  UINT i = swapChain_->GetCurrentBackBufferIndex();
  D3D12_RESOURCE_BARRIER b{};
  b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  b.Transition.pResource = swapChainResources_[i].Get();
  b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  b.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  commandList_->ResourceBarrier(1, &b);
}