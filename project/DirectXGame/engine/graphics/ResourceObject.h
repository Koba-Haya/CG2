#pragma once
#include <d3d12.h>
#include <wrl.h>

class ResourceObject {
public:
	using ComPtr = Microsoft::WRL::ComPtr<ID3D12Resource>;

	ResourceObject() = default;
	explicit ResourceObject(ComPtr resource) : resource_(std::move(resource)) {}

	ID3D12Resource* Get() const { return resource_.Get(); }
	ID3D12Resource* const* GetAddressOf() const { return resource_.GetAddressOf(); }
	ComPtr& GetComPtr() { return resource_; }

private:
	ComPtr resource_;
};
