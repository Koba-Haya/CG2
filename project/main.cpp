#include "GameApp.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	D3DResourceLeakChecker leakChecker;
  GameApp app;
  return app.Run();
}