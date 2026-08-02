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

    void Serialize(nlohmann::json& j) const override {
        j["modelPath"] = modelPath_;
        j["texturePath"] = texturePath_;
        j["environmentCoefficient"] = environmentCoefficient_;
    }
    void Deserialize(const nlohmann::json& j) override {
        if (j.contains("modelPath")) {
            std::string path = j["modelPath"].get<std::string>();
            if (!path.empty() && path != "/") LoadModel(path);
        }
        if (j.contains("texturePath")) {
            std::string path = j["texturePath"].get<std::string>();
            if (!path.empty() && path != "/") LoadTexture(path);
        }
        if (j.contains("environmentCoefficient")) {
            SetEnvironmentCoefficient(j["environmentCoefficient"].get<float>());
        }
    }

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

    void Update(float deltaTime) override;
    void Draw() override;

private:
    std::string modelPath_;
    std::string texturePath_;
    std::unique_ptr<ModelInstance> modelInstance_;
    // LoadModel()で置き換えられた直前のモデル（とそのテクスチャ）を1フレームだけ延命させる。
    // 同一フレーム内（コマンドリストがCloseされる前）に破棄すると、直前のロード処理で
    // 記録されたテクスチャアップロードコマンドが参照するリソースが消え、
    // D3D12 ERROR: OBJECT_DELETED_WHILE_STILL_IN_USE でクラッシュするため。
    std::unique_ptr<ModelInstance> pendingDestroyModelInstance_;
    float environmentCoefficient_ = 0.0f;
};

} // namespace AbsoluteEngine
