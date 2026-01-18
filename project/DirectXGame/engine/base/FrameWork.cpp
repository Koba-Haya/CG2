#include "Framework.h"
#include <Windows.h>
#include <objbase.h>
#include <string>

static void FatalBoxAndTerminate_(const char* msg) {
	MessageBoxA(nullptr, msg, "Fatal", MB_OK | MB_ICONERROR);
	std::terminate();
}

static void CheckBoolOrDie_(bool ok, const char* what) {
	if (!ok) {
		FatalBoxAndTerminate_(what);
	}
}

static void CheckHROrDie_(HRESULT hr, const char* what) {
	if (FAILED(hr)) {
		std::string s = std::string(what) + " (HRESULT failed)";
		FatalBoxAndTerminate_(s.c_str());
	}
}

void AbsoluteFrameWork::InitializeEngine_() {
	// COM: Engine側で統一（GameAppからは消す）
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	CheckHROrDie_(hr, "CoInitializeEx failed");

	// Window / DX
	winApp_.Initialize();

	DirectXCommon::InitParams params{
		winApp_.GetHInstance(),
		winApp_.GetHwnd(),
		WinApp::kClientWidth,
		WinApp::kClientHeight,
	};
	dx_.Initialize(params);

	// Managers: Engine services
	TextureManager::GetInstance()->Initialize(&dx_);
	ModelManager::GetInstance()->Initialize(&dx_, &dx_.GetSrvAllocator());
	ParticleManager::GetInstance()->Initialize(&dx_);

	// ImGui / Input / Audio
	imgui_.Initialize(&winApp_, &dx_);

	const bool inputOk = input_.Initialize(winApp_.GetHInstance(), winApp_.GetHwnd());
	CheckBoolOrDie_(inputOk, "input_.Initialize failed");

	const bool audioOk = audio_.Initialize();
	CheckBoolOrDie_(audioOk, "audio_.Initialize failed");
}

void AbsoluteFrameWork::FinalizeEngine_() {
	// App layer finalize first (game resources release)
	// -> This function is called after OnFinalize(), so only engine services remain.

	// ImGui first (uses DX)
	imgui_.Finalize();

	// Audio / Input
	audio_.Shutdown();
	input_.Finalize();

	// Managers (release GPU resources before DX destruction)
	ParticleManager::GetInstance()->Finalize();

	// Window
	winApp_.Finalize();

	// COM
	CoUninitialize();
}

void AbsoluteFrameWork::Run() {
	endRequest_ = false;

	InitializeEngine_();
	Initialize();

	while (!endRequest_) {
		// Close button, etc.
		if (!winApp_.ProcessMessage()) {
			RequestEnd();
			break;
		}

		input_.Update();
		Update();
		Draw();
	}

	Finalize();
	FinalizeEngine_();
}

void AbsoluteFrameWork::Initialize() {
	// 何もしない（アプリ側がoverrideする）
}

void AbsoluteFrameWork::Finalize() {
	// 何もしない（アプリ側がoverrideする）
}

void AbsoluteFrameWork::Update() {
	// 何もしない（アプリ側がoverrideする）
}
