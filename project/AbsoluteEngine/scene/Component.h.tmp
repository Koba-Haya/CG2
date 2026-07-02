#pragma once
#include <memory>
#include <string>
#include "../../externals/nlohmann/json.hpp" // JSONライブラリのインクルード

namespace AbsoluteEngine {

class GameObject;

class IComponent {
public:
    virtual ~IComponent() = default;

    virtual void Update(float deltaTime) {}
    virtual void Draw() {}
    
    // コンポーネントの種類名（ファクトリやシリアライズ用）
    virtual std::string GetTypeName() const = 0;

    // シリアライズ（保存・復元）用インターフェース
    virtual void Serialize(nlohmann::json& j) const {}
    virtual void Deserialize(const nlohmann::json& j) {}

    void SetOwner(GameObject* owner) { owner_ = owner; }
    GameObject* GetOwner() const { return owner_; }

protected:
    GameObject* owner_ = nullptr;
};

} // namespace AbsoluteEngine
