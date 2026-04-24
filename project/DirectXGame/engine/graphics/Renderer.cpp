#include "Renderer.h"
#include "DirectXCommon.h"
#include "ModelInstance.h"
#include "Sprite.h"
#include "ParticleManager.h"
#include "ShaderCompilerUtils.h" // Assuming we need this or similar for dxc
#include <cassert>

void Renderer::Initialize(DirectXCommon* dx) {
    assert(dx);
    dx_ = dx;

    auto* device = dx_->GetDevice();
    auto* utils = dx_->GetDXCUtils();
    auto* compiler = dx_->GetDXCCompiler();
    auto* includeHandler = dx_->GetDXCIncludeHandler();

    // Init 3D Object Pipelines
    {
        PipelineDesc desc = UnifiedPipeline::MakeObject3DDesc();
        objPipelineOpaque_ = std::make_unique<UnifiedPipeline>();
        objPipelineOpaque_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.fillMode = D3D12_FILL_MODE_WIREFRAME;
        objPipelineWireframe_ = std::make_unique<UnifiedPipeline>();
        objPipelineWireframe_->Initialize(device, utils, compiler, includeHandler, desc);
    }

    // Init Sprite Pipelines
    {
        PipelineDesc desc = UnifiedPipeline::MakeSpriteDesc();
        
        desc.blendMode = BlendMode::Alpha;
        spritePipelineAlpha_ = std::make_unique<UnifiedPipeline>();
        spritePipelineAlpha_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.blendMode = BlendMode::Add;
        spritePipelineAdd_ = std::make_unique<UnifiedPipeline>();
        spritePipelineAdd_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.blendMode = BlendMode::Subtract;
        spritePipelineSub_ = std::make_unique<UnifiedPipeline>();
        spritePipelineSub_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.blendMode = BlendMode::Multiply;
        spritePipelineMul_ = std::make_unique<UnifiedPipeline>();
        spritePipelineMul_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.blendMode = BlendMode::Screen;
        spritePipelineScreen_ = std::make_unique<UnifiedPipeline>();
        spritePipelineScreen_->Initialize(device, utils, compiler, includeHandler, desc);
    }

    // Init Particle Pipelines
    {
        PipelineDesc desc = UnifiedPipeline::MakeParticleDesc();

        desc.blendMode = BlendMode::Alpha;
        particlePipelineAlpha_ = std::make_unique<UnifiedPipeline>();
        particlePipelineAlpha_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.blendMode = BlendMode::Add;
        particlePipelineAdd_ = std::make_unique<UnifiedPipeline>();
        particlePipelineAdd_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.blendMode = BlendMode::Subtract;
        particlePipelineSub_ = std::make_unique<UnifiedPipeline>();
        particlePipelineSub_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.blendMode = BlendMode::Multiply;
        particlePipelineMul_ = std::make_unique<UnifiedPipeline>();
        particlePipelineMul_->Initialize(device, utils, compiler, includeHandler, desc);

        desc.blendMode = BlendMode::Screen;
        particlePipelineScreen_ = std::make_unique<UnifiedPipeline>();
        particlePipelineScreen_->Initialize(device, utils, compiler, includeHandler, desc);
    }
}

void Renderer::SetViewProjection(const Matrix4x4& view, const Matrix4x4& proj) {
    view_ = view;
    proj_ = proj;
}

void Renderer::SetCameraCB(ID3D12Resource* cb) {
    cameraCB_ = cb;
}

void Renderer::SetDirectionalLightCB(ID3D12Resource* cb) {
    directionalLightCB_ = cb;
}

void Renderer::SetPointLightCB(ID3D12Resource* cb) {
    pointLightCB_ = cb;
}

void Renderer::SetSpotLightCB(ID3D12Resource* cb) {
    spotLightCB_ = cb;
}

void Renderer::DrawModel(ModelInstance* model) {
    if (!model) return;
    auto* cmdList = dx_->GetCommandList();

    UnifiedPipeline* pipeline = model->IsWireframe() ? objPipelineWireframe_.get() : objPipelineOpaque_.get();
    pipeline->SetPipelineState(cmdList);

    model->Draw(cmdList, view_, proj_, directionalLightCB_, cameraCB_, pointLightCB_, spotLightCB_);
}

void Renderer::DrawSprite(Sprite* sprite) {
    if (!sprite) return;
    auto* cmdList = dx_->GetCommandList();

    UnifiedPipeline* pipeline = GetSpritePipeline_(sprite->GetBlendMode());
    pipeline->SetPipelineState(cmdList);

    sprite->Draw(cmdList, view_, proj_);
}

void Renderer::DrawParticles(ParticleManager* pm, BlendMode blendMode) {
    if (!pm) return;
    auto* cmdList = dx_->GetCommandList();

    UnifiedPipeline* pipeline = GetParticlePipeline_(blendMode);
    pm->Draw(cmdList, pipeline);
}

UnifiedPipeline* Renderer::GetSpritePipeline_(BlendMode mode) {
    switch (mode) {
    case BlendMode::Add: return spritePipelineAdd_.get();
    case BlendMode::Subtract: return spritePipelineSub_.get();
    case BlendMode::Multiply: return spritePipelineMul_.get();
    case BlendMode::Screen: return spritePipelineScreen_.get();
    case BlendMode::Alpha:
    case BlendMode::Opaque:
    default:
        return spritePipelineAlpha_.get();
    }
}

UnifiedPipeline* Renderer::GetParticlePipeline_(BlendMode mode) {
    switch (mode) {
    case BlendMode::Add: return particlePipelineAdd_.get();
    case BlendMode::Subtract: return particlePipelineSub_.get();
    case BlendMode::Multiply: return particlePipelineMul_.get();
    case BlendMode::Screen: return particlePipelineScreen_.get();
    case BlendMode::Alpha:
    case BlendMode::Opaque:
    default:
        return particlePipelineAlpha_.get();
    }
}
