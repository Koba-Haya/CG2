#pragma once
#include "Vector.h"
#include <string>
#include <vector>
#include <optional>

struct LevelData {
    struct ColliderData {
        std::string type;
        Vector3 center;
        Vector3 size;
    };

    struct ObjectData {
        std::string type;
        std::string name;
        Vector3 translation;
        Vector3 rotation;
        Vector3 scaling;
        std::optional<std::string> fileName;
        std::optional<ColliderData> collider;
        std::vector<ObjectData> children;
    };

    std::vector<ObjectData> objects;
};
