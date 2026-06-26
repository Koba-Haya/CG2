#include "EnemyShootComponent.h"
#include "AbsoluteEngine/scene/BaseScene.h"
#include "AbsoluteEngine/scene/ModelComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "../../actor/Bullet/BulletComponent.h"
#include "../../actor/Player/PlayerComponent.h"
#include <cmath>

void EnemyShootComponent::Update(float deltaTime) {
    if (!owner_) return;

    shootTimer_ += deltaTime;
    if (shootTimer_ >= shootInterval_) {
        shootTimer_ -= shootInterval_;
        
        auto scene = BaseScene::GetActiveScene();
        if (!scene) return;
        
        std::shared_ptr<AbsoluteEngine::GameObject> playerObj = nullptr;
        for (const auto& obj : scene->GetRootObjects()) {
            if (obj && obj->GetComponent<PlayerComponent>()) {
                playerObj = obj;
                break;
            }
        }
        
        if (playerObj) {
            Vector3 spawnPos = owner_->GetTransform().translate;
            Vector3 targetPos = playerObj->GetTransform().translate;
            
            Vector3 diff = { targetPos.x - spawnPos.x, targetPos.y - spawnPos.y, targetPos.z - spawnPos.z };
            float len = std::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
            Vector3 dir = {0,0,0};
            if (len > 0.0f) dir = {diff.x/len, diff.y/len, diff.z/len};
            Vector3 vel = { dir.x * 20.0f, dir.y * 20.0f, dir.z * 20.0f };
            
            auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("EnemyBullet");
            auto bulletModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
            bulletModelComp->LoadModel("resources/app/bullet/bullet.obj");
            bulletObj->AddComponent(std::move(bulletModelComp));
            bulletObj->GetTransform().translate = spawnPos;
            
            auto bulletComp = std::make_unique<BulletComponent>();
            bulletComp->Initialize(vel);
            bulletObj->AddComponent(std::move(bulletComp));
            
            scene->AddRootObject(bulletObj);
        }
    }
}
