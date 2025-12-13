#pragma once

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
#include "AABB.h"

#include <fstream>
#include <list>
#include <string>

// Transform は main.cpp と同じ定義をここに寄せたい
struct Transform {
  Vector3 scale;
  Vector3 rotate;
  Vector3 translate;
};

// パーティクル
struct Particle {
  Transform transform;
  Vector3 velocity; // 移動速度 (units/sec)
  float lifetime;   // 寿命 (秒)
  float age;        // 経過時間 (秒)
  Vector4 color;    // RGBA
};

// 加速度フィールド
struct AccelerationField {
  Vector3 acceleration; // 加速度
  AABB area;            // 範囲（AABB）
};

// 形状
enum class EmitterShape {
  Box,   // AABB 的な直方体
  Sphere // 球
};

enum class ParticleColorMode {
  RandomRGB, // 完全ランダム(RGB)
  RangeRGB,  // 指定色 ± 幅 (RGB 空間)
  RangeHSV,  // 指定色 ± 幅 (HSV 空間)
  Fixed      // 完全固定色
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

  void RespawnParticle_(Particle &p); // パーティクル再生成
  Vector4 GenerateParticleColor_();   // パーティクル色生成

  Matrix4x4 MakeBillboardMatrix(const Vector3 &scale, const Vector3 &translate,
                                const Matrix4x4 &viewMatrix);

  bool IsCollision(const AABB &aabb, const Vector3 &point);

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
  UnifiedPipeline objPipeline_;              // Object3D 用
  UnifiedPipeline emitterGizmoPipelineWire_; // エミッタ用 ワイヤーフレーム
  UnifiedPipeline spritePipelineAlpha_;      // スプライト用 Alpha
  UnifiedPipeline spritePipelineAdd_;        // スプライト用 Add
  UnifiedPipeline spritePipelineSub_;        // スプライト用 Sub
  UnifiedPipeline spritePipelineMul_;        // スプライト用 Mul
  UnifiedPipeline spritePipelineScreen_;     // スプライト用 Screen
  UnifiedPipeline particlePipelineAlpha_;    // パーティクル用 Alpha
  UnifiedPipeline particlePipelineAdd_;      // パーティクル用 Add
  UnifiedPipeline particlePipelineSub_;      // パーティクル用 Sub
  UnifiedPipeline particlePipelineMul_;      // パーティクル用 Mul
  UnifiedPipeline particlePipelineScreen_;   // パーティクル用 Screen

  // SRV 割り当てヘルパ（DirectXCommon の SRV ヒープを利用）
  SrvAllocator srvAlloc_;

  // モデル・スプライト
  Model model_;
  Model planeModel_;
  Sprite sprite_;
  Model emitterSphereModel_;
  Model emitterBoxModel_;

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

  // パーティクル関連
  static constexpr uint32_t kParticleCount_ =
      100; // GPU側の最大インスタンス数 (= 最大発生数上限)

  // パーティクルのリスト（CPU 側で管理）
  std::list<Particle> particles_;

  // GPU に詰めた個数（Draw() で使う）
  uint32_t activeParticleCount_ = 0;

  // Emitter の上限（ImGui で 0〜kParticleCount_ を操作）
  uint32_t particleCountUI_ = kParticleCount_; // 「Max Count」の意味に変える

  // 初期に一気に出す数（起動時だけ利用）
  uint32_t initialParticleCount_ = 30;

  // === パーティクル制御用パラメータ（Emitterの挙動） ===
  Vector3 particleSpawnCenter_{0.0f, 0.0f, 0.0f}; // 生成中心
  Vector3 particleSpawnExtent_{1.0f, 1.0f, 1.0f}; // 中心からの±範囲

  Vector3 particleBaseDir_{0.0f, 1.0f, 0.0f}; // 基本移動方向
  float particleDirRandomness_ = 0.5f;        // 方向のばらつき

  float particleSpeedMin_ = 0.5f; // 速度レンジ
  float particleSpeedMax_ = 2.0f;

  float particleLifeMin_ = 1.0f; // 寿命レンジ
  float particleLifeMax_ = 3.0f;

  // 1 秒あたりの発生数（頻度）と蓄積値
  float particleEmitRate_ = 10.0f; // particles / sec
  float particleEmitAccum_ = 0.0f; // Emit の蓄積カウンタ

  // エミッタの形状
  EmitterShape emitterShape_ = EmitterShape::Box;
  bool showEmitterGizmo_ = false; // 範囲表示のON/OFF

  enum class ParticleColorMode {
    RandomRGB, // 完全ランダム(RGB)
    RangeRGB,  // 指定色 ± 幅 (RGB 空間)
    RangeHSV,  // 指定色 ± 幅 (HSV 空間)
    Fixed      // 完全固定色
  };

  ParticleColorMode particleColorMode_ = ParticleColorMode::RandomRGB;

  // 基本色（RGBA）
  Vector4 particleBaseColor_{1.0f, 1.0f, 1.0f, 1.0f};

  // RGB 空間での幅 (0〜1)
  Vector3 particleColorRangeRGB_{0.2f, 0.2f, 0.2f};

  // HSV 空間での中心値 & 幅 (H,S,V それぞれ 0〜1)
  Vector3 particleBaseHSV_{0.0f, 1.0f, 1.0f};
  Vector3 particleHSVRange_{0.1f, 0.1f, 0.1f};

  // 平行光用 CB
  ComPtr<ID3D12Resource> directionalLightCB_;
  DirectionalLight *directionalLightData_ = nullptr;

  // Transform / カメラ
  Transform transform_;       // 球
  Transform cameraTransform_; // カメラ
  Transform transformSprite_; // スプライト
  Transform uvTransformSprite_;
  Transform transform2_; // 平面

  Matrix4x4 view3D_;
  Matrix4x4 proj3D_;

  // DebugCamera は後で unique_ptr にする想定
  class DebugCamera *debugCamera_ = nullptr;

  // ライティング・ブレンドモード
  int lightingMode_ = 1;
  int spriteBlendMode_ = 0;
  int particleBlendMode_ = 0;
  bool useMonsterBall_ = true;
  float selectVol_ = 1.0f;

  // 板ポリ用のVB/IBビューをメンバに追加
  D3D12_VERTEX_BUFFER_VIEW particleVBView_{};
  D3D12_INDEX_BUFFER_VIEW particleIBView_{};
  ComPtr<ID3D12Resource> particleVB_;
  ComPtr<ID3D12Resource> particleIB_;

  // パーティクル用テクスチャ
  ComPtr<ID3D12Resource> particleTexture_;
  D3D12_GPU_DESCRIPTOR_HANDLE particleTextureHandle_{};

  // 加速度フィールド
  AccelerationField accelerationField_;
  bool enableAccelerationField_ = false;
};