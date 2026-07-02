#pragma once
#include "Component.h"
#include "../Type/Vector.h"

namespace AbsoluteEngine {

class RigidbodyComponent : public IComponent {
public:
    RigidbodyComponent() = default;
    ~RigidbodyComponent() override = default;

    std::string GetTypeName() const override { return "RigidbodyComponent"; }

    void Serialize(nlohmann::json& j) const override;
    void Deserialize(const nlohmann::json& j) override;

    void Update(float deltaTime) override;
    void DrawInspectorUI() override;

    // 物理パラメータ
    void SetVelocity(const Vector3& v) { velocity_ = v; }
    Vector3 GetVelocity() const { return velocity_; }

    void AddForce(const Vector3& force);
    
    void SetMass(float m) { mass_ = m; }
    float GetMass() const { return mass_; }

    void SetDrag(float d) { drag_ = d; }
    float GetDrag() const { return drag_; }

    void SetUseGravity(bool use) { useGravity_ = use; }
    bool GetUseGravity() const { return useGravity_; }

    void SetKinematic(bool k) { isKinematic_ = k; }
    bool IsKinematic() const { return isKinematic_; }

private:
    Vector3 velocity_{0.0f, 0.0f, 0.0f};
    Vector3 acceleration_{0.0f, 0.0f, 0.0f};

    float mass_ = 1.0f;
    float drag_ = 0.0f; // 空気抵抗
    bool useGravity_ = true;
    bool isKinematic_ = false; // trueなら物理演算（重力や反発）の影響を受けない

    static constexpr float GRAVITY = -9.8f;
};

} // namespace AbsoluteEngine
