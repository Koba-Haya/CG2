#include "ModelComponent.h"
#include "GameObject.h"
#include "Transform.h"
#include "DissolveComponent.h"
#include "../../resources/AssetManager.h"

namespace AbsoluteEngine {

void ModelComponent::LoadModel(const std::string& path) {
    auto res = AssetManager::GetInstance()->Load<ModelResource>(path);
    if (!res) return;

    modelPath_ = path;
    modelInstance_ = std::make_unique<ModelInstance>();
    ModelInstance::CreateInfo ci{};
    ci.resource = res;
    ci.baseColor = {1, 1, 1, 1};
    ci.lightingMode = 1;
    ci.environmentCoefficient = environmentCoefficient_;
    modelInstance_->Initialize(ci);
    
    if (!texturePath_.empty()) {
        LoadTexture(texturePath_);
    }
}

void ModelComponent::LoadTexture(const std::string& path) {
    texturePath_ = path;
    if (modelInstance_) {
        auto tex = AssetManager::GetInstance()->Load<TextureResource>(path);
        if (tex) {
            modelInstance_->SetOverrideTexture(tex);
        }
    }
}

void ModelComponent::Draw() {
    if (modelInstance_ && owner_) {
        const Transform& transform = owner_->GetTransform();
        Matrix4x4 world = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
        
        modelInstance_->SetWorld(world);

        auto dissolve = owner_->GetComponent<DissolveComponent>();
        if (dissolve) {
            modelInstance_->SetDissolveParam(dissolve->enable, dissolve->threshold, dissolve->edgeRange, dissolve->edgeColor, dissolve->maskColor);
        }
        
        modelInstance_->Draw();
    }
}

} // namespace AbsoluteEngine
