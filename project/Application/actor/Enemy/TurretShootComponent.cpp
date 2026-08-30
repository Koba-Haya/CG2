#include "TurretShootComponent.h"
#include "AbsoluteEngine/scene/BaseScene.h"
#include "AbsoluteEngine/scene/ModelComponent.h"
#include "AbsoluteEngine/scene/ColliderComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "../../actor/Bullet/BulletComponent.h"
#include "../../actor/Player/PlayerComponent.h"
#include "GameCamera.h"
#include <cmath>

void TurretShootComponent::Update(float deltaTime) {
    if (!owner_) return;

    if (burstShotsRemaining_ > 0) {
        // バースト中: burstDelay_間隔で残弾を消化する（クールダウンタイマーは進めない）
        burstTimer_ += deltaTime;
        if (burstTimer_ >= burstDelay_) {
            burstTimer_ -= burstDelay_;
            Vector3 dir;
            if (CanFire(dir)) {
                FireOneBullet(dir);
            }
            --burstShotsRemaining_;
        }
        return;
    }

    cooldownTimer_ += deltaTime;
    if (cooldownTimer_ < shootInterval_) return;

    // クールダウン完了。画面内にいてプレイヤーが見つかった瞬間にバーストを開始する
    Vector3 dir;
    if (!CanFire(dir)) return;

    cooldownTimer_ = 0.0f;
    FireOneBullet(dir);
    burstShotsRemaining_ = burstCount_ - 1;
    burstTimer_ = 0.0f;
}

bool TurretShootComponent::CanFire(Vector3& outToPlayerDir) const {
    if (!owner_) return false;

    auto scene = BaseScene::GetActiveScene();
    if (!scene) return false;

    auto* camera = scene->GetMainCamera();
    if (!camera) return false;

    const Vector3 currentPos = owner_->GetTransform().translate;

    // --- 画面内チェック（NDC ±1.2 マージン内にいる場合のみ発射を許可）---
    Matrix4x4 viewMat = camera->GetViewMatrix();
    Matrix4x4 projMat = camera->GetProjectionMatrix();
    Matrix4x4 vpMat   = Multiply(viewMat, projMat);

    float w = currentPos.x * vpMat.m[0][3] + currentPos.y * vpMat.m[1][3]
            + currentPos.z * vpMat.m[2][3] + vpMat.m[3][3];
    if (w <= 0.01f) return false; // カメラの背後にいる場合は発射しない

    float ndcX = (currentPos.x * vpMat.m[0][0] + currentPos.y * vpMat.m[1][0]
               + currentPos.z * vpMat.m[2][0] + vpMat.m[3][0]) / w;
    float ndcY = (currentPos.x * vpMat.m[0][1] + currentPos.y * vpMat.m[1][1]
               + currentPos.z * vpMat.m[2][1] + vpMat.m[3][1]) / w;

    constexpr float ndcMargin = 1.2f;
    if (ndcX < -ndcMargin || ndcX > ndcMargin || ndcY < -ndcMargin || ndcY > ndcMargin) return false;

    // --- プレイヤーを探す ---
    std::shared_ptr<AbsoluteEngine::GameObject> playerObj = nullptr;
    for (const auto& obj : scene->GetRootObjects()) {
        if (obj && obj->GetComponent<PlayerComponent>()) {
            playerObj = obj;
            break;
        }
    }
    if (!playerObj) return false;

    // --- プレイヤー方向（タレットは非移動のため、進行方向ではなく毎回直接この方向を狙う）---
    Vector3 playerPos = playerObj->GetTransform().translate;
    Vector3 toPlayer = {
        playerPos.x - currentPos.x,
        playerPos.y - currentPos.y,
        playerPos.z - currentPos.z
    };
    float toPlayerLen = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
    if (toPlayerLen < 0.001f) return false;

    outToPlayerDir = { toPlayer.x / toPlayerLen, toPlayer.y / toPlayerLen, toPlayer.z / toPlayerLen };
    return true;
}

void TurretShootComponent::FireOneBullet(const Vector3& dir) {
    auto scene = BaseScene::GetActiveScene();
    if (!scene || !owner_) return;

    const Vector3 spawnPos = owner_->GetTransform().translate;
    const Vector3 vel = { dir.x * 20.0f, dir.y * 20.0f, dir.z * 20.0f };

    auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("EnemyBullet");
    bulletObj->SetTag("EnemyBullet");

    auto bulletModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
    bulletModelComp->LoadModel("resources/app/bullet/bullet.obj");
    bulletObj->AddComponent(std::move(bulletModelComp));

    auto colliderComp = std::make_unique<AbsoluteEngine::ColliderComponent>();
    colliderComp->type = AbsoluteEngine::ColliderComponent::Type::Sphere;
    colliderComp->radius = 0.5f;
    bulletObj->AddComponent(std::move(colliderComp));

    bulletObj->GetTransform().translate = spawnPos;

    auto bulletComp = std::make_unique<BulletComponent>();
    bulletComp->Initialize(vel);
    bulletObj->AddComponent(std::move(bulletComp));

    scene->AddRootObject(bulletObj);
}
