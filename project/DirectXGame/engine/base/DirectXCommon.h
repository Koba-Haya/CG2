#pragma once
#include "externals/DirectXTex/d3dx12.h"
#include "SrvAllocator.h"
#include "DirectXResourceUtils.h"
#include <Windows.h>
#include <dxgi1_6.h>
#include <wrl.h>

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

class DirectXCommon {
public:
	// namespace省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	struct InitParams {
		HINSTANCE hInstance;
		HWND hwnd;
		int32_t clientWidth;
		int32_t clientHeight;
	};

public:
	DirectXCommon();
	~DirectXCommon();

	// スクショ相当の初期化（デバイス/コマンド/スワップチェーン/深度/各種ヒープ/RTV初期化/DSV初期化/フェンス/ビューポート/シザー/DCX/ImGui）
	void Initialize(const InitParams& params);

	// フレームの最初と最後（必要なら使用）
	void BeginFrame();
	void EndFrame();

	// 参照用のゲッタ
	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const {
		return commandList_.Get();
	}
	ID3D12CommandAllocator* GetCommandAllocator() const {
		return commandAllocator_.Get();
	}
	ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }
	IDXGISwapChain4* GetSwapChain() const { return swapChain_.Get(); }
	ID3D12DescriptorHeap* GetSRVHeap() const { return srvDescriptorHeap_.Get(); }
	ID3D12DescriptorHeap* GetRTVHeap() const { return rtvDescriptorHeap_.Get(); }
	ID3D12DescriptorHeap* GetDSVHeap() const { return dsvDescriptorHeap_.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(int index) const {
		return rtvHandles_[index];
	}
	const D3D12_VIEWPORT& GetViewport() const { return viewport_; }
	const D3D12_RECT& GetScissorRect() const { return scissorRect_; }

	IDxcUtils* GetDXCUtils() const { return dxc_.utils; }
	IDxcCompiler3* GetDXCCompiler() const { return dxc_.compiler; }
	IDxcIncludeHandler* GetDXCIncludeHandler() const {
		return dxc_.includeHandler;
	}

	// SRV割当器取得
	SrvAllocator& GetSrvAllocator() { return *srvAlloc_; }
	// const版SRV割当器取得
	const SrvAllocator& GetSrvAllocator() const { return *srvAlloc_; }

	UINT GetSRVDescriptorSize() const { return descriptorSizeSRV_; }
	int GetBackBufferCount() const { return swapChainDesc_.BufferCount; }
	DXGI_FORMAT GetRTVFormat() const { return rtvDesc_.Format; }

private:
	void CreateDeviceAndFactory_();
	void ChooseAdapter_();
	void CreateCommandObjects_();
	void CreateSwapChain_();
	void CreateDescriptorHeaps_();
	void CreateRTVs_();
	void CreateDepthStencil_();
	void CreateFenceAndEvent_();
	void SetupViewportAndScissor_();
	void InitDXC_();
	/*void InitImGui_();*/
	void TransitionBackBufferToRenderTarget_();
	void TransitionBackBufferToPresent_();

private:
	// 初期化入力
	HINSTANCE hInstance_ = nullptr;
	HWND hwnd_ = nullptr;
	int32_t clientWidth_ = 0;
	int32_t clientHeight_ = 0;

	// DXGI/D3D
	ComPtr<IDXGIFactory7> dxgiFactory_;
	ComPtr<IDXGIAdapter4> useAdapter_;
	ComPtr<ID3D12Device> device_;
	ComPtr<ID3D12CommandQueue> commandQueue_;
	ComPtr<ID3D12CommandAllocator> commandAllocator_;
	ComPtr<ID3D12GraphicsCommandList> commandList_;
	ComPtr<IDXGISwapChain4> swapChain_;

	// バックバッファ
	ComPtr<ID3D12Resource> swapChainResources_[2];
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2]{};

	// ヒープ
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
	uint32_t descriptorSizeRTV_ = 0;
	uint32_t descriptorSizeSRV_ = 0;
	uint32_t descriptorSizeDSV_ = 0;

	// SRV割当器
	std::unique_ptr<SrvAllocator> srvAlloc_;

	// 深度ステンシル
	ComPtr<ID3D12Resource> depthStencilResource_;

	// フェンス
	ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	// ビューポート/シザー
	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};

	// DXC
	struct DXCBlock {
		struct IDxcUtils* utils = nullptr;
		struct IDxcCompiler3* compiler = nullptr;
		struct IDxcIncludeHandler* includeHandler = nullptr;
	} dxc_{};

	// 一時
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
};