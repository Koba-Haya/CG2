// EngineCore.h
#pragma once
#include "AssetLoader.h"
#include "Audio.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "ResourceManager.h"
#include "ShaderCompiler.h"
#include "UnifiedPipeline.h"
#include "WinApp.h"

class EngineCore {
public:
  EngineCore();
  ~EngineCore();

  void Initialize();
  void BeginFrame();
  void EndFrame();
  void Finalize();

  // 各マネージャ・コンポーネント取得
  WinApp *GetWinApp() { return &winApp_; }
  DirectXCommon *GetDX() { return &dxCommon_; }
  ResourceManager *GetResourceManager() { return &resourceManager_; }
  ShaderCompiler *GetShaderCompiler() { return &shaderCompiler_; }
  Input *GetInput() { return &input_; }
  AudioManager *GetAudio() { return &audio_; }
  AssetLoader *GetAssets() { return &assetLoader_; }

private:
  WinApp winApp_;
  DirectXCommon dxCommon_;
  ResourceManager resourceManager_;
  ShaderCompiler shaderCompiler_;
  Input input_;
  AudioManager audio_;
  AssetLoader assetLoader_;
  bool initialized_ = false;
};
