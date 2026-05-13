#pragma once
#include <cstdint>

struct ID3D12GraphicsCommandList;

class WinApp;
class DirectXCommon;

class ImGuiManager {
public:
	ImGuiManager() = default;
	~ImGuiManager() = default;

	void Initialize(WinApp* winApp, DirectXCommon* dx);
	void Finalize();

	void Begin();
	void End();
	void Draw(ID3D12GraphicsCommandList* cmdList);

private:
	bool initialized_ = false;
	bool frameBegun_ = false;

	WinApp* winApp_ = nullptr;
	DirectXCommon* dx_ = nullptr;
};
