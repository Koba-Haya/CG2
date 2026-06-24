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
};

} // namespace AbsoluteEngine
