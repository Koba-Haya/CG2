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

  struct Particle {
    Transform transform;
    Vector3 velocity; // 移動速度 (units/sec)
    float lifetime;   // 寿命 (秒)
    float age;        // 経過時間 (秒)
    Vector4 color;    // RGBA
  };
  void RespawnParticle_(Particle &p);

  Matrix4x4 MakeBillboardMatrix(const Vector3 &scale,
                                       const Vector3 &translate,
                                       const Matrix4x4 &viewMatrix);

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
  static constexpr uint32_t kParticleCount_ = 10;
  Particle particles_[kParticleCount_];

  ComPtr<ID3D12Resource> particleInstanceBuffer_;
  D3D12_GPU_DESCRIPTOR_HANDLE particleMatricesSrvGPU_{};
  struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
  };
  ParticleForGPU *particleMatrices_ = nullptr;

  // パーティクル用マテリアル定数バッファ
  ComPtr<ID3D12Resource> particleMaterialCB_;
  // Object3D/Particle の HLSL Material と同じレイアウトに合わせる
  struct ParticleMaterialData {
    Vector4 color; // RGBA
    int32_t enableLighting;
    float pad[3]; // アラインメント合わせ
    Matrix4x4 uvTransform;
  };
  ParticleMaterialData *particleMaterialData_ = nullptr;

  // === パーティクル制御用パラメータ（ImGuiから操作） ===
  uint32_t particleCountUI_ = kParticleCount_; // 表示する最大個数

  Vector3 particleSpawnCenter_{0.0f, 0.0f, 0.0f}; // 生成中心
  Vector3 particleSpawnExtent_{1.0f, 1.0f, 1.0f}; // 中心からの±範囲

  Vector3 particleBaseDir_{0.0f, 1.0f, 0.0f}; // 基本移動方向
  float particleDirRandomness_ = 0.5f;        // 方向のばらつき

  float particleSpeedMin_ = 0.5f; // 速度レンジ
  float particleSpeedMax_ = 2.0f;

  float particleLifeMin_ = 1.0f; // 寿命レンジ
  float particleLifeMax_ = 3.0f;

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

  // 板ポリ用のVB/IBビューをメンバに追加
  D3D12_VERTEX_BUFFER_VIEW particleVBView_{};
  D3D12_INDEX_BUFFER_VIEW  particleIBView_{};
  ComPtr<ID3D12Resource>   particleVB_;
  ComPtr<ID3D12Resource>   particleIB_;

  // パーティクル用テクスチャ
  ComPtr<ID3D12Resource> particleTexture_;
  D3D12_GPU_DESCRIPTOR_HANDLE particleTextureHandle_{};
};