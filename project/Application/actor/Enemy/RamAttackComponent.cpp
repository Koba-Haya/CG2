#include "RamAttackComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"
#include "EnemyComponent.h"
#include "../../actor/Player/PlayerComponent.h"

void RamAttackComponent::OnCollision(AbsoluteEngine::GameObject* other) {
    if (!owner_ || !other) return;

    if (!other->GetComponent<PlayerComponent>()) return;

    // プレイヤーに衝突した瞬間に確実に自爆撃破する（現在のHPに関わらず）
    if (auto* enemyComp = owner_->GetComponent<EnemyComponent>()) {
        enemyComp->OnHit(enemyComp->GetMaxHp());
    }
}
