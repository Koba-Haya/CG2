#include "CollisionManager.h"
#include "GameObject.h"
#include "ColliderComponent.h"
#include "Component.h"
#include <cmath>
#include <algorithm>

namespace AbsoluteEngine {

void CollisionManager::CollectColliders(std::shared_ptr<GameObject> obj, std::vector<ColliderComponent*>& outColliders) {
    if (!obj || !obj->IsActive()) return;

    auto collider = obj->GetComponent<ColliderComponent>();
    if (collider && collider->type != ColliderComponent::Type::None) {
        outColliders.push_back(collider);
    }

    for (auto& child : obj->GetChildren()) {
        CollectColliders(child, outColliders);
    }
}

void CollisionManager::Update(const std::vector<std::shared_ptr<GameObject>>& rootObjects) {
    std::vector<ColliderComponent*> colliders;
    for (const auto& obj : rootObjects) {
        CollectColliders(obj, colliders);
    }

    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            ColliderComponent* a = colliders[i];
            ColliderComponent* b = colliders[j];

            if (CheckCollision(a, b)) {
                // 各コンポーネントのOnCollisionを呼び出す
                for (const auto& comp : a->GetOwner()->GetComponents()) {
                    comp->OnCollision(b->GetOwner());
                }
                for (const auto& comp : b->GetOwner()->GetComponents()) {
                    comp->OnCollision(a->GetOwner());
                }
            }
        }
    }
}

bool CollisionManager::CheckCollision(ColliderComponent* a, ColliderComponent* b) {
    Vector3 posA = a->GetOwner()->GetTransform().translate;
    posA.x += a->centerOffset.x; posA.y += a->centerOffset.y; posA.z += a->centerOffset.z;
    Vector3 posB = b->GetOwner()->GetTransform().translate;
    posB.x += b->centerOffset.x; posB.y += b->centerOffset.y; posB.z += b->centerOffset.z;

    if (a->type == ColliderComponent::Type::Sphere && b->type == ColliderComponent::Type::Sphere) {
        float distSq = (posA.x - posB.x) * (posA.x - posB.x) + 
                       (posA.y - posB.y) * (posA.y - posB.y) + 
                       (posA.z - posB.z) * (posA.z - posB.z);
        float rSq = (a->radius + b->radius) * (a->radius + b->radius);
        return distSq <= rSq;
    }
    
    if (a->type == ColliderComponent::Type::AABB && b->type == ColliderComponent::Type::AABB) {
        return (std::abs(posA.x - posB.x) <= a->size.x + b->size.x) &&
               (std::abs(posA.y - posB.y) <= a->size.y + b->size.y) &&
               (std::abs(posA.z - posB.z) <= a->size.z + b->size.z);
    }

    // Sphere vs AABB
    ColliderComponent* sphere = a->type == ColliderComponent::Type::Sphere ? a : b;
    ColliderComponent* aabb = a->type == ColliderComponent::Type::AABB ? a : b;
    if (sphere->type == ColliderComponent::Type::Sphere && aabb->type == ColliderComponent::Type::AABB) {
        Vector3 posS = sphere->GetOwner()->GetTransform().translate;
        posS.x += sphere->centerOffset.x; posS.y += sphere->centerOffset.y; posS.z += sphere->centerOffset.z;
        Vector3 posBox = aabb->GetOwner()->GetTransform().translate;
        posBox.x += aabb->centerOffset.x; posBox.y += aabb->centerOffset.y; posBox.z += aabb->centerOffset.z;
        
        float closestX = std::max(posBox.x - aabb->size.x, std::min(posS.x, posBox.x + aabb->size.x));
        float closestY = std::max(posBox.y - aabb->size.y, std::min(posS.y, posBox.y + aabb->size.y));
        float closestZ = std::max(posBox.z - aabb->size.z, std::min(posS.z, posBox.z + aabb->size.z));

        float distSq = (closestX - posS.x) * (closestX - posS.x) +
                       (closestY - posS.y) * (closestY - posS.y) +
                       (closestZ - posS.z) * (closestZ - posS.z);
        return distSq <= (sphere->radius * sphere->radius);
    }

    return false;
}

} // namespace AbsoluteEngine
