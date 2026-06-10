#include "PlaneHitEffect.h"
#include "camera/Camera.h"
#include "graphics/Renderer.h"
#include "math/Method.h"

PlaneHitEffect::PlaneHitEffect(std::shared_ptr<ModelResource> modelRes, const Vector3& position) {
    transform_.translate = position;
    transform_.scale = {1.0f, 1.0f, 1.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    
    lifetime_ = 0.5f;
    age_ = 0.0f;

    instance_.Initialize({modelRes, {1.0f, 1.0f, 1.0f, 1.0f}, 0});
}

void PlaneHitEffect::Update(float deltaTime, const Camera* camera) {
    age_ += deltaTime;
    if (age_ >= lifetime_) {
        Kill();
        return;
    }

    float t = age_ / lifetime_;
    
    float scaleVal = t * 5.0f;
    Vector3 currentScale = {scaleVal, scaleVal, scaleVal};
    
    Matrix4x4 viewMatrix = camera ? camera->GetViewMatrix() : MakeIdentity4x4();
    Matrix4x4 billBoardMat = viewMatrix;
    billBoardMat.m[3][0] = 0.0f;
    billBoardMat.m[3][1] = 0.0f;
    billBoardMat.m[3][2] = 0.0f;
    billBoardMat = Inverse(billBoardMat); 

    transform_.rotate.z += 5.0f * deltaTime;
    Matrix4x4 rotZ = MakeRotateZMatrix(transform_.rotate.z);
    
    Matrix4x4 rotMat = Multiply(rotZ, billBoardMat);
    Matrix4x4 scaleMat = MakeScaleMatrix(currentScale);
    
    Matrix4x4 worldMat = Multiply(scaleMat, rotMat);
    worldMat.m[3][0] = transform_.translate.x;
    worldMat.m[3][1] = transform_.translate.y;
    worldMat.m[3][2] = transform_.translate.z;

    instance_.SetWorld(worldMat);
    instance_.SetColor({1.0f, 1.0f, 1.0f, 1.0f - t});
}

void PlaneHitEffect::Draw() {
    Renderer::GetInstance()->DrawEffectModel(&instance_);
}
