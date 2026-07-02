#pragma once
#include <vector>
#include <memory>

#include "../Type/Vector.h"

namespace AbsoluteEngine {

class GameObject;
class ColliderComponent;

class CollisionManager {
public:
    static CollisionManager& GetInstance() {
        static CollisionManager instance;
        return instance;
    }

    void Update(const std::vector<std::shared_ptr<GameObject>>& rootObjects);

private:
    CollisionManager() = default;
    ~CollisionManager() = default;

    struct CollisionResult {
        bool isHit = false;
        Vector3 normal{0, 0, 0};   // BからAに向かう法線ベクトル (Aを押し出す方向)
        float depth = 0.0f;        // めり込みの深さ
    };

    void CollectColliders(std::shared_ptr<GameObject> obj, std::vector<ColliderComponent*>& outColliders);
    CollisionResult CheckCollision(ColliderComponent* a, ColliderComponent* b);
    void ResolvePenetration(ColliderComponent* a, ColliderComponent* b, const CollisionResult& result);
};

} // namespace AbsoluteEngine
