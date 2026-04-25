#define NOMINMAX
#include "Renderer.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "ModelInstance.h"
#include "Sprite.h"
#include "ParticleManager.h"
#include "Skybox.h"
#include "ShaderCompilerUtils.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#define CHECK_INIT(call) { bool _res = (call); if(!_res) throw std::runtime_error(#call " failed!"); }

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
        CHECK_INIT(objPipelineOpaque_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.fillMode = D3D12_FILL_MODE_WIREFRAME;
        objPipelineWireframe_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(objPipelineWireframe_->Initialize(device, utils, compiler, includeHandler, desc));
    }

    // Init Skybox Pipeline
    {
        PipelineDesc desc = UnifiedPipeline::MakeSkyboxDesc();
        skyboxPipeline_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(skyboxPipeline_->Initialize(device, utils, compiler, includeHandler, desc));
    }

    // Init Sprite Pipelines
    {
        PipelineDesc desc = UnifiedPipeline::MakeSpriteDesc();
        
        desc.blendMode = BlendMode::Alpha;
        spritePipelineAlpha_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(spritePipelineAlpha_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.blendMode = BlendMode::Add;
        spritePipelineAdd_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(spritePipelineAdd_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.blendMode = BlendMode::Subtract;
        spritePipelineSub_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(spritePipelineSub_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.blendMode = BlendMode::Multiply;
        spritePipelineMul_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(spritePipelineMul_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.blendMode = BlendMode::Screen;
        spritePipelineScreen_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(spritePipelineScreen_->Initialize(device, utils, compiler, includeHandler, desc));
    }

    // Init Particle Pipelines
    {
        PipelineDesc desc = UnifiedPipeline::MakeParticleDesc();

        desc.blendMode = BlendMode::Alpha;
        particlePipelineAlpha_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(particlePipelineAlpha_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.blendMode = BlendMode::Add;
        particlePipelineAdd_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(particlePipelineAdd_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.blendMode = BlendMode::Subtract;
        particlePipelineSub_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(particlePipelineSub_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.blendMode = BlendMode::Multiply;
        particlePipelineMul_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(particlePipelineMul_->Initialize(device, utils, compiler, includeHandler, desc));

        desc.blendMode = BlendMode::Screen;
        particlePipelineScreen_ = std::make_unique<UnifiedPipeline>();
        CHECK_INIT(particlePipelineScreen_->Initialize(device, utils, compiler, includeHandler, desc));
    }

    auto align256 = [](size_t size) -> size_t {
        return (size + 255) & ~255;
    };

    cameraCB_ = CreateUploadBuffer_(align256(sizeof(CameraForGPU)));
    cameraCB_->Map(0, nullptr, reinterpret_cast<void**>(&cameraMapped_));

    directionalLightCB_ = CreateUploadBuffer_(align256(sizeof(DirectionalLightGroupCB)));
    directionalLightCB_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightMapped_));

    pointLightCB_ = CreateUploadBuffer_(align256(sizeof(PointLightGroupCB)));
    pointLightCB_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightMapped_));

    spotLightCB_ = CreateUploadBuffer_(align256(sizeof(SpotLightGroupCB)));
    spotLightCB_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightMapped_));
}

// ===== 高レベル API =====

void Renderer::SetCamera(const Camera& camera) {
    view_ = camera.GetViewMatrix();
    proj_ = camera.GetProjectionMatrix();

    if (cameraMapped_) {
        Matrix4x4 invView = Inverse(view_);
        cameraMapped_->worldPosition = {invView.m[3][0], invView.m[3][1], invView.m[3][2]};
        cameraMapped_->pad = 0.0f;
    }
}

void Renderer::SetDirectionalLights(const std::vector<DirLight>& lights, bool groupEnabled) {
    if (!directionalLightMapped_) return;

    std::memset(directionalLightMapped_, 0, sizeof(DirectionalLightGroupCB));
    int count = static_cast<int>(std::min(lights.size(), static_cast<size_t>(kMaxDirLights)));
    directionalLightMapped_->count = count;
    directionalLightMapped_->enabled = groupEnabled ? 1 : 0;

    for (int i = 0; i < count; ++i) {
        const auto& src = lights[i];
        auto& dst = directionalLightMapped_->lights[i];
        dst.color[0] = src.color.x; dst.color[1] = src.color.y; dst.color[2] = src.color.z; dst.color[3] = 1.0f;
        dst.direction[0] = src.direction.x; dst.direction[1] = src.direction.y; dst.direction[2] = src.direction.z;
        dst.intensity = src.intensity;
        dst.enabled = src.enabled ? 1 : 0;
    }
}

void Renderer::SetPointLights(const std::vector<PointLight>& lights, bool groupEnabled) {
    if (!pointLightMapped_) return;

    std::memset(pointLightMapped_, 0, sizeof(PointLightGroupCB));
    int count = static_cast<int>(std::min(lights.size(), static_cast<size_t>(kMaxPointLights)));
    pointLightMapped_->count = count;
    pointLightMapped_->enabled = groupEnabled ? 1 : 0;

    for (int i = 0; i < count; ++i) {
        const auto& src = lights[i];
        auto& dst = pointLightMapped_->lights[i];
        dst.color[0] = src.color.x; dst.color[1] = src.color.y; dst.color[2] = src.color.z; dst.color[3] = 1.0f;
        dst.position[0] = src.position.x; dst.position[1] = src.position.y; dst.position[2] = src.position.z;
        dst.intensity = src.intensity;
        dst.radius = src.radius;
        dst.decay = src.decay;
        dst.enabled = src.enabled ? 1 : 0;
    }
}

void Renderer::SetSpotLights(const std::vector<SpotLight>& lights, bool groupEnabled) {
    if (!spotLightMapped_) return;

    std::memset(spotLightMapped_, 0, sizeof(SpotLightGroupCB));
    int count = static_cast<int>(std::min(lights.size(), static_cast<size_t>(kMaxSpotLights)));
    spotLightMapped_->count = count;
    spotLightMapped_->enabled = groupEnabled ? 1 : 0;

    static constexpr float kPi = 3.14159265f;
    for (int i = 0; i < count; ++i) {
        const auto& src = lights[i];
        auto& dst = spotLightMapped_->lights[i];
        dst.color[0] = src.color.x; dst.color[1] = src.color.y; dst.color[2] = src.color.z; dst.color[3] = 1.0f;
        dst.position[0] = src.position.x; dst.position[1] = src.position.y; dst.position[2] = src.position.z;
        dst.direction[0] = src.direction.x; dst.direction[1] = src.direction.y; dst.direction[2] = src.direction.z;
        dst.intensity = src.intensity;
        dst.distance = src.distance;
        dst.decay = src.decay;
        dst.cosAngle = std::cos(src.coneAngleDeg * (kPi / 180.0f));
        dst.enabled = src.enabled ? 1 : 0;
    }
}

float Renderer::GetScreenWidth() const {
    assert(dx_);
    return dx_->GetViewport().Width;
}

float Renderer::GetScreenHeight() const {
    assert(dx_);
    return dx_->GetViewport().Height;
}

float Renderer::GetAspectRatio() const {
    float h = GetScreenHeight();
    return (h != 0.0f) ? (GetScreenWidth() / h) : 1.0f;
}

// ===== 描画メソッド =====

void Renderer::DrawModel(ModelInstance* model) {
    if (!model) return;
    auto* cmdList = dx_->GetCommandList();

    UnifiedPipeline* pipeline = model->IsWireframe() ? objPipelineWireframe_.get() : objPipelineOpaque_.get();
    pipeline->SetPipelineState(cmdList);

    // Root Params: 
    // 0: Material, 1: Transform, 2: Texture, 3: DirLight, 4: Camera, 5: PointLight, 6: SpotLight
    if (directionalLightCB_) {
        cmdList->SetGraphicsRootConstantBufferView(3, directionalLightCB_->GetGPUVirtualAddress());
    }
    if (cameraCB_) {
        cmdList->SetGraphicsRootConstantBufferView(4, cameraCB_->GetGPUVirtualAddress());
    }
    if (pointLightCB_) {
        cmdList->SetGraphicsRootConstantBufferView(5, pointLightCB_->GetGPUVirtualAddress());
    }
    if (spotLightCB_) {
        cmdList->SetGraphicsRootConstantBufferView(6, spotLightCB_->GetGPUVirtualAddress());
    }

    model->DrawInternal(cmdList, view_, proj_);
}

void Renderer::DrawSprite(Sprite* sprite) {
    if (!sprite) return;
    auto* cmdList = dx_->GetCommandList();

    UnifiedPipeline* pipeline = GetSpritePipeline_(sprite->GetBlendMode());
    pipeline->SetPipelineState(cmdList);

    sprite->DrawInternal(cmdList, view_, proj_);
}

void Renderer::DrawSkybox(Skybox* skybox) {
    if (!skybox) return;
    auto* cmdList = dx_->GetCommandList();

    UnifiedPipeline* pipeline = skyboxPipeline_.get();
    if (!pipeline) return;

    pipeline->SetPipelineState(cmdList);
    skybox->DrawInternal(cmdList);
}

void Renderer::DrawParticles(ParticleManager* pm, BlendMode blendMode) {
    if (!pm) return;
    auto* cmdList = dx_->GetCommandList();

    UnifiedPipeline* pipeline = GetParticlePipeline_(blendMode);
    pipeline->SetPipelineState(cmdList);
    pm->DrawInternal(cmdList);
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

Microsoft::WRL::ComPtr<ID3D12Resource> Renderer::CreateUploadBuffer_(size_t size) {
    assert(dx_);
    auto device = dx_->GetDevice();
    Microsoft::WRL::ComPtr<ID3D12Resource> res;
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                    IID_PPV_ARGS(&res));
    return res;
}
