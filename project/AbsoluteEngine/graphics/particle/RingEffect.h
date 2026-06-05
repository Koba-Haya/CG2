#pragma once
#include "BaseEffect.h"
#include "graphics/primitive/Ring.h"

class TextureResource;

class RingEffect : public BaseEffect {
public:
    RingEffect(ID3D12Device* device, TextureResource* texture, const Vector3& position);
    ~RingEffect() override = default;

    void Update(float deltaTime, const Camera* camera) override;
    void Draw() override;

private:
    Ring ring_;
    Ring::Params ringParams_{};
    TextureResource* texture_ = nullptr;
    float maxOuterRadius_ = 4.0f;
};
