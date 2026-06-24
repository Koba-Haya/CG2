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
};

} // namespace AbsoluteEngine
