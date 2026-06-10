#pragma once
#include "BaseEffect.h"
#include "graphics/primitive/Cylinder.h"

class TextureResource;

class CylinderEffect : public BaseEffect {
public:
    CylinderEffect(ID3D12Device* device, TextureResource* texture, const Vector3& position);
    ~CylinderEffect() override = default;

    void Update(float deltaTime, const Camera* camera) override;
    void Draw() override;

private:
    Cylinder cylinder_;
    Cylinder::Params cylinderParams_{};
    TextureResource* texture_ = nullptr;
    float maxHeight_ = 8.0f;
};
