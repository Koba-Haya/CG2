#pragma once
#include "BaseEffect.h"
#include "graphics/3d/model/ModelInstance.h"

class PlaneHitEffect : public BaseEffect {
public:
    PlaneHitEffect(std::shared_ptr<ModelResource> modelRes, const Vector3& position);
    ~PlaneHitEffect() override = default;

    void Update(float deltaTime, const Camera* camera) override;
    void Draw() override;

private:
    ModelInstance instance_;
};
