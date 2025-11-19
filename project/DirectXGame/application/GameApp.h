#pragma once

#include <fstream>
#include <string>

#include "Audio.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "Model.h"
#include "ShaderCompiler.h"
#include "Sprite.h"
#include "UnifiedPipeline.h"
#include "Vector.h"
#include "WinApp.h"
#include "include.h" // Transform で Vector3/Matrix4x4 使うので

// Transform は main.cpp と同じ定義をここに寄せたい
struct Transform {
  Vector3 scale;
  Vector3 rotate;
  Vector3 translate;
};

// D3D リソースリークチェック（元 main.cpp の struct を流用）
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

class GameApp {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  GameApp();
  ~GameApp();

  // 全体の初期化
  bool Initialize();

  // メインループを回す
  int Run();

  // 明示的終了処理
  void Finalize();

private:
  // 1フレーム分の更新・描画。Run() から呼ばれる。
  void Update();
  void Draw();

  // 今後 Step3 で使う初期化系を分けておくと整理しやすい
  void InitPipelines_(); // UnifiedPipeline 関係
  void InitResources_(); // Model/Sprite/Particle 等
  void InitLogging_();   // ログファイル作成
  void InitCamera_();    // Transform / DebugCamera 等（後で）

private:
  WinApp winApp_;
  DirectXCommon dx_;
  Input input_;
  AudioManager audio_;

  std::ofstream logStream_;
  D3DResourceLeakChecker leakChecker_; // ReportLiveObjects

  // ===== ここから Step3 用のメンバ（まだ中身は後で詰める） =====

  // ShaderCompiler（main.cpp の CompileShader を置き換えていく）
  ShaderCompiler shaderCompiler_;

  // パイプライン
  UnifiedPipeline objPipeline_;
  UnifiedPipeline spritePipelineAlpha_;
  UnifiedPipeline spritePipelineAdd_;
  UnifiedPipeline spritePipelineSub_;
  UnifiedPipeline spritePipelineMul_;
  UnifiedPipeline spritePipelineScreen_;
  UnifiedPipeline particlePipeline_;

  // SRV 割り当てヘルパ（DirectXCommon の SRV ヒープを利用）
  SrvAllocator srvAlloc_;

  // モデル・スプライト
  Model model_;
  Model planeModel_;
  Sprite sprite_;

  // パーティクル関連
  struct Particle {
    Transform transform;
  };
  static constexpr uint32_t kParticleCount_ = 10;
  Particle particles_[kParticleCount_];

  ComPtr<ID3D12Resource> particleInstanceBuffer_;
  D3D12_GPU_DESCRIPTOR_HANDLE particleMatricesSrvGPU_{};
  struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
  };
  TransformationMatrix *particleMatrices_ = nullptr;

  // 平行光用 CB
  ComPtr<ID3D12Resource> directionalLightCB_;
  DirectionalLight *directionalLightData_ = nullptr;

  // Transform / カメラ
  Transform transform_;       // 球
  Transform cameraTransform_; // カメラ
  Transform transformSprite_; // スプライト
  Transform uvTransformSprite_;
  Transform transform2_; // 平面

  // DebugCamera は後で unique_ptr にする想定
  class DebugCamera *debugCamera_ = nullptr;

  // ライティング・ブレンドモード
  int lightingMode_ = 1;
  int spriteBlendMode_ = 0;
  bool useMonsterBall_ = true;
  float selectVol_ = 1.0f;
};