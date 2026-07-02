#pragma once
#include "Component.h"
#include "Transform.h"
#include <string>

namespace AbsoluteEngine {

class LightNodeComponent : public IComponent {
public:
    enum class Type { None, Directional, Point, Spot };

    LightNodeComponent() = default;
    ~LightNodeComponent() override = default;

    std::string GetTypeName() const override { return "LightNodeComponent"; }

    Type type = Type::None;
    Vector3 color = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float radius = 10.0f;       // Point, Spot用
    float decay = 2.0f;         // Point, Spot用
    float distance = 10.0f;     // Spot用
    float coneAngleDeg = 30.0f; // Spot用

    void Serialize(nlohmann::json& j) const override {
        j["lightType"] = static_cast<int>(type);
        j["color"] = { {"x", color.x}, {"y", color.y}, {"z", color.z} };
        j["intensity"] = intensity;
        j["radius"] = radius;
        j["decay"] = decay;
        j["distance"] = distance;
        j["coneAngleDeg"] = coneAngleDeg;
    }
    void Deserialize(const nlohmann::json& j) override {
        if (j.contains("lightType")) type = static_cast<Type>(j["lightType"].get<int>());
        if (j.contains("color")) {
            color.x = j["color"].value("x", 1.0f);
            color.y = j["color"].value("y", 1.0f);
            color.z = j["color"].value("z", 1.0f);
        }
        if (j.contains("intensity")) intensity = j["intensity"].get<float>();
        if (j.contains("radius")) radius = j["radius"].get<float>();
        if (j.contains("decay")) decay = j["decay"].get<float>();
        if (j.contains("distance")) distance = j["distance"].get<float>();
        if (j.contains("coneAngleDeg")) coneAngleDeg = j["coneAngleDeg"].get<float>();
    }
};

} // namespace AbsoluteEngine
