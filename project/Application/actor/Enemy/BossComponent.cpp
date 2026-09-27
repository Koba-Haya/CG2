#include "BossComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "AbsoluteEngine/scene/BaseScene.h"
#include "AbsoluteEngine/scene/ModelComponent.h"
#include "AbsoluteEngine/scene/ColliderComponent.h"
#include "../Bullet/BulletComponent.h"
#include "../Player/PlayerComponent.h"
#include <cmath>

namespace {
// フェーズごとの攻撃間隔・弾速（マジックナンバーをここに集約し、調整しやすくする）
constexpr float kAimedIntervalPhase1 = 1.6f;
constexpr float kAimedIntervalPhase2 = 1.3f;
constexpr float kAimedIntervalPhase3 = 0.8f;
constexpr float kAimedSpeed = 24.0f;

constexpr float kSpreadIntervalPhase2 = 2.2f;
constexpr float kSpreadIntervalPhase3 = 1.5f;
constexpr float kSpreadSpeed = 16.0f;
constexpr int   kSpreadCountPhase2 = 5;
constexpr int   kSpreadCountPhase3 = 8;
constexpr float kSpreadAngleDeg = 12.0f;

constexpr float kSpinSpeed = 0.6f; // rad/sec
}

void BossComponent::Update(float deltaTime) {
    if (!isActive_ || !owner_) return;

    // 完全に静止した箱に見えないよう、常時ゆっくり自転させる（演出コスト最小のリアクション）
    spinTimer_ += deltaTime * kSpinSpeed;
    owner_->GetTransform().rotate.y = spinTimer_;

    UpdatePhase();
    UpdateAttack(deltaTime);
}

void BossComponent::UpdatePhase() {
    if (maxHp_ <= 0) return;
    const float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
    if (hpRatio <= 1.0f / 3.0f) {
        phase_ = BossPhase::Phase3;
    } else if (hpRatio <= 2.0f / 3.0f) {
        phase_ = BossPhase::Phase2;
    } else {
        phase_ = BossPhase::Phase1;
    }
}

void BossComponent::UpdateAttack(float deltaTime) {
    aimedShotTimer_ += deltaTime;
    spreadShotTimer_ += deltaTime;

    float aimedInterval = kAimedIntervalPhase1;
    switch (phase_) {
        case BossPhase::Phase1: aimedInterval = kAimedIntervalPhase1; break;
        case BossPhase::Phase2: aimedInterval = kAimedIntervalPhase2; break;
        case BossPhase::Phase3: aimedInterval = kAimedIntervalPhase3; break;
    }
    if (aimedShotTimer_ >= aimedInterval) {
        aimedShotTimer_ -= aimedInterval;
        FireAimedShot(kAimedSpeed);
    }

    // 扇状弾はPhase2以降のみ（Phase1は単発弾のみで動きを覚えてもらう）
    if (phase_ == BossPhase::Phase2 || phase_ == BossPhase::Phase3) {
        const float spreadInterval = (phase_ == BossPhase::Phase3) ? kSpreadIntervalPhase3 : kSpreadIntervalPhase2;
        if (spreadShotTimer_ >= spreadInterval) {
            spreadShotTimer_ -= spreadInterval;
            const int count = (phase_ == BossPhase::Phase3) ? kSpreadCountPhase3 : kSpreadCountPhase2;
            FireSpreadShot(count, kSpreadAngleDeg, kSpreadSpeed);
        }
    }
}

namespace {
// 発射位置(spawnPos)から速度(vel)の直進弾を1発生成してアクティブシーンに追加する
void SpawnBossBullet(const Vector3& spawnPos, const Vector3& vel) {
    auto scene = BaseScene::GetActiveScene();
    if (!scene) return;

    auto bulletObj = std::make_shared<AbsoluteEngine::GameObject>("BossBullet");
    bulletObj->SetTag("EnemyBullet");

    auto bulletModelComp = std::make_unique<AbsoluteEngine::ModelComponent>();
    bulletModelComp->LoadModel("resources/app/bullet/bullet.obj");
    bulletModelComp->SetColor({1.0f, 0.2f, 0.6f, 1.0f}); // ボス弾は他の敵弾と区別できるよう色を変える
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
} // namespace

void BossComponent::FireAimedShot(float speed) {
    if (!owner_) return;
    auto scene = BaseScene::GetActiveScene();
    if (!scene) return;

    std::shared_ptr<AbsoluteEngine::GameObject> playerObj;
    for (const auto& obj : scene->GetRootObjects()) {
        if (obj && obj->GetComponent<PlayerComponent>()) {
            playerObj = obj;
            break;
        }
    }
    if (!playerObj) return;

    const Vector3 spawnPos = owner_->GetTransform().translate;
    Vector3 toPlayer = {
        playerObj->GetTransform().translate.x - spawnPos.x,
        playerObj->GetTransform().translate.y - spawnPos.y,
        playerObj->GetTransform().translate.z - spawnPos.z
    };
    const float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
    if (len < 0.001f) return;
    toPlayer.x /= len; toPlayer.y /= len; toPlayer.z /= len;

    SpawnBossBullet(spawnPos, { toPlayer.x * speed, toPlayer.y * speed, toPlayer.z * speed });
}

void BossComponent::FireSpreadShot(int count, float spreadAngleDeg, float speed) {
    if (!owner_ || count <= 0) return;
    auto scene = BaseScene::GetActiveScene();
    if (!scene) return;

    std::shared_ptr<AbsoluteEngine::GameObject> playerObj;
    for (const auto& obj : scene->GetRootObjects()) {
        if (obj && obj->GetComponent<PlayerComponent>()) {
            playerObj = obj;
            break;
        }
    }
    if (!playerObj) return;

    const Vector3 spawnPos = owner_->GetTransform().translate;
    Vector3 toPlayer = {
        playerObj->GetTransform().translate.x - spawnPos.x,
        playerObj->GetTransform().translate.y - spawnPos.y,
        playerObj->GetTransform().translate.z - spawnPos.z
    };
    const float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
    if (len < 0.001f) return;
    toPlayer.x /= len; toPlayer.y /= len; toPlayer.z /= len;

    // プレイヤーは主にXY平面（画面平面）上を動くため、Z軸周りに回転させて扇状に振り分ける。
    // 中心(0発分)から見て左右対称になるよう、-half〜+halfの角度で等間隔に配置する。
    const float spreadRad = spreadAngleDeg * 3.14159265f / 180.0f;
    const float half = static_cast<float>(count - 1) * 0.5f;
    for (int i = 0; i < count; ++i) {
        const float angle = (static_cast<float>(i) - half) * spreadRad;
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        Vector3 dir = {
            toPlayer.x * c - toPlayer.y * s,
            toPlayer.x * s + toPlayer.y * c,
            toPlayer.z
        };
        SpawnBossBullet(spawnPos, { dir.x * speed, dir.y * speed, dir.z * speed });
    }
}

void BossComponent::TakeDamage(int damage) {
    if (!isActive_) return;

    hp_ -= damage;
    if (hp_ <= 0) {
        hp_ = 0;
        isActive_ = false; // 撃破
        if (owner_) {
            owner_->Destroy();
        }
    }
}

void BossComponent::OnCollision(AbsoluteEngine::GameObject* other) {
    if (!isActive_ || !other) return;

    if (other->GetName().find("PlayerBullet") != std::string::npos || other->GetTag() == "PlayerBullet") {
        TakeDamage(1);
    }
}
