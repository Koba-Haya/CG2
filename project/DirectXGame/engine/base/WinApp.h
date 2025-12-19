#pragma once
#include <Windows.h>
#include <functional>

class WinApp {
public:
	static const int kClientWidth = 1280;
	static const int kClientHeight = 720;

	using MessageHandler = std::function<bool(HWND, UINT, WPARAM, LPARAM)>;

	WinApp() = default;
	~WinApp() = default;

	void Initialize();
	void Finalize();

	HINSTANCE GetHInstance() const { return hInstance_; }
	HWND GetHwnd() const { return hwnd_; }

	bool ProcessMessage();

	// WinAppはImGuiを知らない。欲しいのは「追加のメッセージ処理」だけ。
	void SetMessageHandler(MessageHandler handler) { messageHandler_ = std::move(handler); }

private:
	static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
	HINSTANCE hInstance_ = nullptr;
	HWND hwnd_ = nullptr;

	MessageHandler messageHandler_;
};
