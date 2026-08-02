#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "GPUParticle.h"
#include "Matrix.h"
#include "BlendMode.h" // ブレンドモード列挙体
#include <vector>

class DirectXCommon;
class TextureResource;

class GPUParticleManager {
public:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    static GPUParticleManager* GetInstance();

    void Initialize(DirectXCommon* dx);
    void Update();
    // blendMode: 描画に使用するブレンドモード（デフォルト Alpha）
    void Draw(BlendMode blendMode = BlendMode::Alpha);

    // --- エミッタ操作API（加点要素: GPU Particle拡張） ---

    using EmitterShape = GPUEmitterShape;

    struct EmitterDesc {
        EmitterShape shape = EmitterShape::Sphere;
        Vector3 translate{ 0.0f, 0.0f, 0.0f };
        Vector3 halfExtents{ 1.0f, 1.0f, 1.0f }; // Box用
        float radius = 1.0f;                     // Sphere用
        uint32_t count = 10;
        float frequency = 0.5f; // 0以下なら自動ループしない（SetEmitterEnabledで手動制御する想定）
    };

    // 常駐エミッタを1つ生成し、そのインデックスを返す（環境演出用）。
    // 予約数を超えた場合は失敗し UINT32_MAX を返す。
    uint32_t CreateEmitter(const EmitterDesc& desc);
    void SetEmitterTransform(uint32_t emitterIndex, const Vector3& translate);
    void SetEmitterEnabled(uint32_t emitterIndex, bool enabled);

    // 内部で予約されたバーストプールをラウンドロビンし、指定位置へワンショットのパーティクル群を発生させる。
    void EmitBurst(const Vector3& position, uint32_t count = 15, float radius = 0.3f);

    using FieldType = GPUFieldType;

    struct FieldDesc {
        FieldType type = FieldType::None;
        Vector3 target{ 0.0f, 0.0f, 0.0f };
        Vector3 direction{ 0.0f, 1.0f, 0.0f };
        float strength = 1.0f;
    };

    // slot は [0, kMaxGPUFields) の範囲で指定する。
    void SetField(int slot, const FieldDesc& desc);

private:
    GPUParticleManager() = default;
    ~GPUParticleManager() = default;

    void CreateResources();
    void CreateComputePipeline();
    void CreateGraphicsPipeline();
    void CreateQuad();

    DirectXCommon* dx_ = nullptr;

    // Resource
    ComPtr<ID3D12Resource> particleBuffer_;
    uint32_t srvIndex_ = 0;
    uint32_t uavIndex_ = 0;

    // Pipelines
    ComPtr<ID3D12RootSignature> computeRootSignature_;
    ComPtr<ID3D12PipelineState> computePipelineState_;

    // Emit/Update共有（b0:Emitter[], b1:PerFrame, b2:Field[], u0-u2）。
    // フレーム内でRootSignatureを切り替えるとD3D12デバッグレイヤーのクラッシュを誘発するため、
    // Emit/Updateで1つのRootSignatureを共有し、PSOのみを切り替える構成にしている。
    ComPtr<ID3D12RootSignature> emitRootSignature_;
    ComPtr<ID3D12PipelineState> emitPipelineState_;
    ComPtr<ID3D12PipelineState> updatePipelineState_;

    ComPtr<ID3D12RootSignature> graphicsRootSignature_;
    // ブレンドモードごとのパイプライン（インデックスは BlendMode の enum 値と対応）
    // [0]=Opaque相当, [1]=Alpha, [2]=Add, [3]=Subtract, [4]=Multiply, [5]=Screen
    static constexpr int kBlendModeCount = 6;
    ComPtr<ID3D12PipelineState> graphicsPipelineStates_[kBlendModeCount];

    // Constant Buffer for PerView
    struct PerView {
        Matrix4x4 viewProjection;
        Matrix4x4 billboardMatrix;
    };
    ComPtr<ID3D12Resource> perViewCB_;
    PerView* perViewMapped_ = nullptr;

    struct PerFrame {
        float time;
        float deltaTime;
    };
    ComPtr<ID3D12Resource> perFrameCB_;
    PerFrame* perFrameMapped_ = nullptr;

    ComPtr<ID3D12Resource> emittersCB_;
    GPUEmitterArray* emittersMapped_ = nullptr;

    ComPtr<ID3D12Resource> fieldsCB_;
    GPUFieldArray* fieldsMapped_ = nullptr;

    // エミッタスロットの割り当て: [0, kPersistentEmitterSlots) は CreateEmitter の常駐用、
    // [kPersistentEmitterSlots, kMaxGPUEmitters) は EmitBurst のラウンドロビン用プール。
    static constexpr uint32_t kBurstPoolSize = 4;
    static constexpr uint32_t kPersistentEmitterSlots = kMaxGPUEmitters - kBurstPoolSize;
    uint32_t persistentEmitterCount_ = 0;
    uint32_t burstCursor_ = 0;
    // EmitBurstで立てたemitフラグは「立てた次のUpdate()」でクリアする(GPUが前フレームの
    // Dispatchを消費し終えているタイミングで安全にクリアするため)。
    bool burstPendingClear_[kBurstPoolSize] = {};

    ComPtr<ID3D12Resource> freeListIndexBuffer_;
    ComPtr<ID3D12Resource> freeListBuffer_;
    uint32_t freeListIndexUavIndex_ = 0;
    uint32_t freeListUavIndex_ = 0;

    std::unique_ptr<TextureResource> texture_;

    // Quad Mesh
    ComPtr<ID3D12Resource> vb_;
    ComPtr<ID3D12Resource> ib_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    static const uint32_t kMaxParticles = 1024;
};
