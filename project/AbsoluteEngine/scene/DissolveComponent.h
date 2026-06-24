#pragma once
#include "Component.h"
#include "Transform.h"
#include <string>

namespace AbsoluteEngine {

class DissolveComponent : public IComponent {
public:
    DissolveComponent() = default;
    ~DissolveComponent() override = default;

    std::string GetTypeName() const override { return "DissolveComponent"; }

    bool enable = false;
    float threshold = 0.5f;
    float edgeRange = 0.03f;
    Vector3 edgeColor = {1.0f, 0.4f, 0.3f};
    Vector3 maskColor = {1.0f, 1.0f, 1.0f};
};

} // namespace AbsoluteEngine
