#include "BulletComponent.h"

#include "AbsoluteEngine/scene/BaseScene.h"
#include "../../component/enemy/EnemyComponent.h"
#include "../../component/enemy/BossComponent.h"
#include "../../actor/Player/PlayerComponent.h"

void BulletComponent::Initialize(const Vector3 &vel) {
  velocity_ = vel;
  isActive_ = true;
  lifeTimer_ = 0.0f;
}

void BulletComponent::Update(float deltaTime) {
  if (!isActive_ || !owner_) return;

  lifeTimer_ += deltaTime;
  if (lifeTimer_ >= maxLife_) {
    isActive_ = false;
    owner_->Destroy(); // 寿命が来たらオブジェクトごと消滅
    return;
  }

  auto& t = owner_->GetTransform();
  t.translate.x += velocity_.x * deltaTime;
  t.translate.y += velocity_.y * deltaTime;
  t.translate.z += velocity_.z * deltaTime;

  // 当たり判定（妥協案：弾自身がシーンの敵を探す）
  auto scene = BaseScene::GetActiveScene();
  if (scene) {
      bool isEnemyBullet = (owner_->GetName() == "EnemyBullet");
      const auto& objects = scene->GetRootObjects();
      for (auto& obj : objects) {
          if (!obj || obj.get() == owner_ || !obj->IsActive()) continue;
          
          if (isEnemyBullet) {
              // 敵の弾はプレイヤーに当たる
              PlayerComponent* pComp = obj->GetComponent<PlayerComponent>();
              if (pComp) {
                  Vector3 objPos = obj->GetTransform().translate;
                  Vector3 diff = { t.translate.x - objPos.x, t.translate.y - objPos.y, t.translate.z - objPos.z };
                  float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                  float rSum = radius_ + 1.0f; // プレイヤーの半径（仮）
                  if (distSq <= rSum * rSum) {
                      pComp->TakeDamage(1);
                      isActive_ = false;
                      owner_->Destroy();
                      return;
                  }
              }
          } else {
              // プレイヤーの弾は敵とボスに当たる
              EnemyComponent* enemyComp = obj->GetComponent<EnemyComponent>();
              BossComponent* bossComp = obj->GetComponent<BossComponent>();
              
              if (enemyComp && enemyComp->IsActive()) {
                  Vector3 objPos = obj->GetTransform().translate;
                  Vector3 diff = { t.translate.x - objPos.x, t.translate.y - objPos.y, t.translate.z - objPos.z };
                  float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                  float rSum = radius_ + enemyComp->GetCollisionRadius();
                  if (distSq <= rSum * rSum) {
                      enemyComp->OnHit();
                      isActive_ = false;
                      owner_->Destroy();
                      return;
                  }
              }
              if (bossComp && bossComp->IsActive()) {
                  Vector3 objPos = obj->GetTransform().translate;
                  Vector3 diff = { t.translate.x - objPos.x, t.translate.y - objPos.y, t.translate.z - objPos.z };
                  float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                  float rSum = radius_ + bossComp->GetCollisionRadius();
                  if (distSq <= rSum * rSum) {
                      bossComp->TakeDamage(1);
                      isActive_ = false;
                      owner_->Destroy();
                      return;
                  }
              }
          }
      }
  }
}
