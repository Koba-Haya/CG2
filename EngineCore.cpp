// EngineCore.cpp
#include "EngineCore.h"
#include <cassert>

EngineCore::EngineCore() {}
EngineCore::~EngineCore() { Finalize(); }

void EngineCore::Initialize() {
  // --- Window ---
  winApp_.Initialize();

  // --- Input ---
  bool inputOk = input_.Initialize(winApp_.GetHInstance(), winApp_.GetHwnd());
  assert(inputOk);

  // --- DirectX ---
  dxCommon_.Initialize({winApp_.GetHInstance(), winApp_.GetHwnd(),
                        winApp_.kClientWidth, winApp_.kClientHeight});

  // --- ResourceManager ---
  resourceManager_.Initialize(dxCommon_.GetDevice());

  // --- ShaderCompiler ---
  shaderCompiler_.Initialize(dxCommon_.GetDXCUtils(),
                             dxCommon_.GetDXCCompiler(),
                             dxCommon_.GetDXCIncludeHandler());

  // --- Audio ---
  bool audioOk = audio_.Initialize();
  assert(audioOk);

  initialized_ = true;
}

void EngineCore::BeginFrame() {
  assert(initialized_);
  dxCommon_.BeginFrame();
}

void EngineCore::EndFrame() { dxCommon_.EndFrame(); }

void EngineCore::Finalize() {
  if (!initialized_)
    return;

  audio_.Shutdown();
  input_.Finalize();
  winApp_.Finalize();

  initialized_ = false;
}
