/*   “デバイス生成 + デバッグレイヤ（任意）”を担当   */

#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// 最小のデバイスラッパ：Device と Adapter を保持し、変換関数を提供
class DirectXDevice {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  DirectXDevice() = default;
  ~DirectXDevice() = default;

  // デバッグレイヤを有効化してから Device を作成する（withDebug=true 推奨）
  bool Initialize(bool withDebug);

  // アダプタ/デバイス取得
  ID3D12Device *Get() const { return device_.Get(); }
  IDXGIAdapter4 *Adapter() const { return adapter_.Get(); }

  // ディスクリプタサイズ（ヒープ種別ごとのインクリメント）
  UINT RtvDescriptorSize() const { return rtvIncSize_; }
  UINT DsvDescriptorSize() const { return dsvIncSize_; }
  UINT CbvSrvUavDescriptorSize() const { return cbvSrvUavIncSize_; }

private:
  bool CreateAdapter_();
  bool CreateDevice_();

private:
  ComPtr<IDXGIFactory7> factory_;
  ComPtr<IDXGIAdapter4> adapter_;
  ComPtr<ID3D12Device> device_;

  UINT rtvIncSize_ = 0;
  UINT dsvIncSize_ = 0;
  UINT cbvSrvUavIncSize_ = 0;

  bool debugEnabled_ = false;
};
