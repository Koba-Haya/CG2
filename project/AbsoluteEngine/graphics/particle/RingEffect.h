#pragma once
#include "BaseEffect.h"
#include "graphics/primitive/Ring.h"
#include <memory>

class TextureResource;

class RingEffect : public BaseEffect {
public:
    // texture は shared_ptr で受け取り、Effect自身が生存期間だけ参照を保持する。
    // EffectManagerはシーンをまたいで生存するシングルトンのため、呼び出し元(GameScene等)が
    // 破棄された後もこのEffectが生きている間はテクスチャが解放されないようにするため。
    RingEffect(ID3D12Device* device, std::shared_ptr<TextureResource> texture, const Vector3& position);
    ~RingEffect() override = default;

    void Update(float deltaTime, const Camera* camera) override;
    void Draw() override;

private:
    Ring ring_;
    Ring::Params ringParams_{};
    std::shared_ptr<TextureResource> texture_;
    float maxOuterRadius_ = 4.0f;
};
