#pragma once

#include <d3d12.h>
#include <memory>
#include <unordered_map>

#include "Matrix.h"
#include "Method.h"
#include "UnifiedPipeline.h"

class DirectXCommon;
class ModelInstance;
class Sprite;
class ParticleManager;

class Renderer {
public:
    static Renderer* GetInstance() {
        static Renderer inst;
        return &inst;
    }

    void Initialize(DirectXCommon* dx);

    // フレーム毎に設定するビュープロジェクションやライトの情報
    void SetViewProjection(const Matrix4x4& view, const Matrix4x4& proj);
    void SetCameraCB(ID3D12Resource* cb);
    void SetDirectionalLightCB(ID3D12Resource* cb);
    void SetPointLightCB(ID3D12Resource* cb);
    void SetSpotLightCB(ID3D12Resource* cb);

    // 描画メソッド群
    void DrawModel(ModelInstance* model);
    void DrawSprite(Sprite* sprite);
    void DrawParticles(ParticleManager* pm, BlendMode blendMode = BlendMode::Alpha);

private:
    Renderer() = default;

    DirectXCommon* dx_ = nullptr;

    Matrix4x4 view_ = MakeIdentity4x4();
    Matrix4x4 proj_ = MakeIdentity4x4();

    ID3D12Resource* cameraCB_ = nullptr;
    ID3D12Resource* directionalLightCB_ = nullptr;
    ID3D12Resource* pointLightCB_ = nullptr;
    ID3D12Resource* spotLightCB_ = nullptr;

    // --- パイプライン群 ---
    std::unique_ptr<UnifiedPipeline> objPipelineOpaque_;
    std::unique_ptr<UnifiedPipeline> objPipelineWireframe_;
    
    std::unique_ptr<UnifiedPipeline> spritePipelineAlpha_;
    std::unique_ptr<UnifiedPipeline> spritePipelineAdd_;
    std::unique_ptr<UnifiedPipeline> spritePipelineSub_;
    std::unique_ptr<UnifiedPipeline> spritePipelineMul_;
    std::unique_ptr<UnifiedPipeline> spritePipelineScreen_;

    std::unique_ptr<UnifiedPipeline> particlePipelineAlpha_;
    std::unique_ptr<UnifiedPipeline> particlePipelineAdd_;
    std::unique_ptr<UnifiedPipeline> particlePipelineSub_;
    std::unique_ptr<UnifiedPipeline> particlePipelineMul_;
    std::unique_ptr<UnifiedPipeline> particlePipelineScreen_;

    UnifiedPipeline* GetSpritePipeline_(BlendMode mode);
    UnifiedPipeline* GetParticlePipeline_(BlendMode mode);
};
