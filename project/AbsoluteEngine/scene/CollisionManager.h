#pragma once
#include <vector>
#include <memory>

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

    void CollectColliders(std::shared_ptr<GameObject> obj, std::vector<ColliderComponent*>& outColliders);
    bool CheckCollision(ColliderComponent* a, ColliderComponent* b);
};

} // namespace AbsoluteEngine
