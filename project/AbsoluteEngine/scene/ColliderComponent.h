#pragma once
#include "Component.h"
#include "Transform.h"
#include <string>

namespace AbsoluteEngine {

class ColliderComponent : public IComponent {
public:
    enum class Type {
        None,
        Sphere,
        AABB
    };

    ColliderComponent() = default;
    ~ColliderComponent() override = default;

    std::string GetTypeName() const override { return "ColliderComponent"; }

    Type type = Type::Sphere;
    Vector3 centerOffset = { 0.0f, 0.0f, 0.0f };
    float radius = 1.0f;
    Vector3 size = { 1.0f, 1.0f, 1.0f };

    void Serialize(nlohmann::json& j) const override {
        j["colliderType"] = static_cast<int>(type);
        j["radius"] = radius;
        j["centerOffset"] = { {"x", centerOffset.x}, {"y", centerOffset.y}, {"z", centerOffset.z} };
        j["size"] = { {"x", size.x}, {"y", size.y}, {"z", size.z} };
    }
    void Deserialize(const nlohmann::json& j) override {
        if (j.contains("colliderType")) type = static_cast<Type>(j["colliderType"].get<int>());
        if (j.contains("radius")) radius = j["radius"].get<float>();
        if (j.contains("centerOffset")) {
            centerOffset.x = j["centerOffset"].value("x", 0.0f);
            centerOffset.y = j["centerOffset"].value("y", 0.0f);
            centerOffset.z = j["centerOffset"].value("z", 0.0f);
        }
        if (j.contains("size")) {
            size.x = j["size"].value("x", 1.0f);
            size.y = j["size"].value("y", 1.0f);
            size.z = j["size"].value("z", 1.0f);
        }
    }
};

} // namespace AbsoluteEngine
