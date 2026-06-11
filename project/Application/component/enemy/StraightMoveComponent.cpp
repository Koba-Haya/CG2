#include "StraightMoveComponent.h"

void StraightMoveComponent::Update(float deltaTime) {
    if (owner_) {
        // Z軸の手前方向（マイナス方向）に移動する
        auto& t = owner_->GetTransform();
        t.translate.z -= speed_ * deltaTime;
    }
}
