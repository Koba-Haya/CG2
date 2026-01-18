#include "GameApp.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#include <wrl.h>
#endif

#ifdef _DEBUG
// 終了時にD3D12の生存オブジェクトを列挙する（mainのスコープ最後で走らせる）
struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};
#endif

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	AbsoluteFrameWork* game = new GameApp();

	game->Run();
	delete game;

#ifdef _DEBUG
	D3DResourceLeakChecker leakChecker;
#endif

	return 0;
}
