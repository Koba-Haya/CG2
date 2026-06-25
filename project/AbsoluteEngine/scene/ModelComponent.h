#pragma once
#include "Component.h"
#include "../graphics/3d/model/ModelInstance.h"
#include <string>
#include <memory>

namespace AbsoluteEngine {

class ModelComponent : public IComponent {
public:
    ModelComponent() = default;
    ~ModelComponent() override = default;

    std::string GetTypeName() const override { return "ModelComponent"; }

    void LoadModel(const std::string& path);
    void LoadTexture(const std::string& path);

    const std::string& GetModelPath() const { return modelPath_; }
    const std::string& GetTexturePath() const { return texturePath_; }

    ModelInstance* GetModelInstance() const { return modelInstance_.get(); }
    
    void SetEnvironmentCoefficient(float c) { 
        environmentCoefficient_ = c; 
        if (modelInstance_) {
            modelInstance_->SetEnvironmentCoefficient(c);
        }
    }
    float GetEnvironmentCoefficient() const { return environmentCoefficient_; }

    void Draw() override;

private:
    std::string modelPath_;
    std::string texturePath_;
    std::unique_ptr<ModelInstance> modelInstance_;
    float environmentCoefficient_ = 0.0f;
};

} // namespace AbsoluteEngine
