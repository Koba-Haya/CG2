#include "CylinderEffect.h"
#include "base/DirectXCommon.h"
#include "camera/Camera.h"
#include "graphics/Renderer.h"
#include "graphics/texture/TextureResource.h"
#include "math/Method.h"

CylinderEffect::CylinderEffect(ID3D12Device* device, TextureResource* texture, const Vector3& position)
    : texture_(texture) {
    transform_.translate = position;
    transform_.scale = {1.0f, 1.0f, 1.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    
    lifetime_ = 0.8f;
    age_ = 0.0f;

    cylinderParams_.divide = 32;
    cylinderParams_.topRadiusX = 1.0f;
    cylinderParams_.topRadiusZ = 1.0f;
    cylinderParams_.bottomRadiusX = 1.0f;
    cylinderParams_.bottomRadiusZ = 1.0f;
    cylinderParams_.height = 0.1f;
    cylinderParams_.colorTop = {1.0f, 1.0f, 1.0f, 1.0f};
    cylinderParams_.colorBottom = {1.0f, 1.0f, 1.0f, 1.0f};
    
    cylinder_.Initialize(device, cylinderParams_);
}

void CylinderEffect::Update(float deltaTime, const Camera* camera) {
    age_ += deltaTime;
    if (age_ >= lifetime_) {
        Kill();
        return;
    }

    float t = age_ / lifetime_;
    
    // 時間経過で上に伸び、広がりながら透明になる
    cylinderParams_.height = maxHeight_ * t;
    float currentRadius = 1.0f + t * 0.5f;
    cylinderParams_.topRadiusX = currentRadius;
    cylinderParams_.topRadiusZ = currentRadius;
    cylinderParams_.bottomRadiusX = currentRadius;
    cylinderParams_.bottomRadiusZ = currentRadius;

    float alpha = 1.0f - t;
    cylinderParams_.colorTop.w = alpha;
    cylinderParams_.colorBottom.w = alpha;

    cylinder_.Update(Renderer::GetInstance()->GetDX()->GetDevice(), cylinderParams_);

    Matrix4x4 worldMat = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 viewMatrix = camera ? camera->GetViewMatrix() : MakeIdentity4x4();
    Matrix4x4 projMatrix = camera ? camera->GetProjectionMatrix() : MakeIdentity4x4();
    
    cylinder_.SetTransform(worldMat, viewMatrix, projMatrix);
    // テクスチャのUVスケール設定（縦方向の線を表現するためU方向にリピート）
    cylinder_.SetMaterial({1.0f, 1.0f, 1.0f, alpha}, MakeScaleMatrix({5.0f, 1.0f, 1.0f}));
}

void CylinderEffect::Draw() {
    if (texture_) {
        Renderer::GetInstance()->DrawCylinder(&cylinder_, texture_->GetSrvGpu());
    }
}
