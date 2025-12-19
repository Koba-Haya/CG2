#include "WinApp.h"
#include <cassert>

void WinApp::Initialize() {
    hInstance_ = GetModuleHandle(nullptr);

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WinApp::StaticWindowProc;
    wc.lpszClassName = L"DirectXGameWindowClass";
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    ATOM atom = RegisterClassEx(&wc);
    assert(atom != 0);

    RECT wrc = { 0, 0, kClientWidth, kClientHeight };
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd_ = CreateWindow(
        wc.lpszClassName,
        L"DirectXGame",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        hInstance_,
        this // ★ここが重要：this を渡して紐づける
    );
    assert(hwnd_ != nullptr);

    ShowWindow(hwnd_, SW_SHOW);
}

void WinApp::Finalize() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool WinApp::ProcessMessage() {
    MSG msg{};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            return false;
        }
    }
    return true;
}

LRESULT CALLBACK WinApp::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    WinApp* app = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
        app = reinterpret_cast<WinApp*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<WinApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (app) {
        return app->WindowProc(hwnd, msg, wp, lp);
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    // 外部ハンドラ（ImGuiなど）がメッセージを食うならここで終了
    if (messageHandler_) {
        if (messageHandler_(hwnd, msg, wp, lp)) {
            return 1;
        }
    }

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
