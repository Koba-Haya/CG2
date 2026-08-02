#include "RingEffect.h"
#include "base/DirectXCommon.h"
#include "camera/Camera.h"
#include "graphics/Renderer.h"
#include "graphics/texture/TextureResource.h"
#include "math/Method.h"

RingEffect::RingEffect(ID3D12Device* device, std::shared_ptr<TextureResource> texture, const Vector3& position)
    : texture_(std::move(texture)) {
    transform_.translate = position;
    transform_.scale = {1.0f, 1.0f, 1.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    
    lifetime_ = 0.5f; // 0.5秒で消滅
    age_ = 0.0f;

    ringParams_.divide = 32;
    ringParams_.innerRadius = 0.0f;
    ringParams_.outerRadius = 1.0f;
    ringParams_.colorOuter = {1.0f, 1.0f, 1.0f, 1.0f};
    ringParams_.colorInner = {1.0f, 1.0f, 1.0f, 1.0f};
    ringParams_.uvVertical = true; // 放射状テクスチャを想定
    
    ring_.Initialize(device, ringParams_);
}

void RingEffect::Update(float deltaTime, const Camera* camera) {
    age_ += deltaTime;
    if (age_ >= lifetime_) {
        Kill();
        return;
    }

    float t = age_ / lifetime_;
    // 時間経過で拡大し、透明になる
    ringParams_.outerRadius = 1.0f + (maxOuterRadius_ - 1.0f) * t;
    float alpha = 1.0f - t;
    ringParams_.colorInner.w = alpha;
    ringParams_.colorOuter.w = alpha;

    ring_.Update(Renderer::GetInstance()->GetDX()->GetDevice(), ringParams_);

    // ビルボード行列の計算
    Matrix4x4 viewMatrix = camera ? camera->GetViewMatrix() : MakeIdentity4x4();
    Matrix4x4 billBoardMat = viewMatrix;
    billBoardMat.m[3][0] = 0.0f;
    billBoardMat.m[3][1] = 0.0f;
    billBoardMat.m[3][2] = 0.0f;
    billBoardMat = Inverse(billBoardMat); // カメラの逆回転

    // 平行移動とスケールを適用
    billBoardMat.m[3][0] = transform_.translate.x;
    billBoardMat.m[3][1] = transform_.translate.y;
    billBoardMat.m[3][2] = transform_.translate.z;
    
    Matrix4x4 scaleMat = MakeScaleMatrix(transform_.scale);
    Matrix4x4 worldMat = Multiply(scaleMat, billBoardMat);

    Matrix4x4 projMatrix = camera ? camera->GetProjectionMatrix() : MakeIdentity4x4();
    ring_.SetTransform(worldMat, viewMatrix, projMatrix);
    ring_.SetMaterial({1.0f, 1.0f, 1.0f, alpha}, MakeIdentity4x4());
}

void RingEffect::Draw() {
    if (texture_) {
        Renderer::GetInstance()->DrawRing(&ring_, texture_->GetSrvGpu());
    }
}
