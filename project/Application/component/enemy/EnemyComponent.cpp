#include "EnemyComponent.h"
#include "AbsoluteEngine/scene/GameObject.h"

void EnemyComponent::Update(float deltaTime) {
    if (!isActive_) return;
    // 今後、HPや状態異常などの処理を追加していく
}

void EnemyComponent::OnHit() {
    // 弾が当たったら非アクティブにする（撃破）
    isActive_ = false;
    
    // GameObjectからこのコンポーネントを外すか、GameObject自体を非表示・削除する処理が必要ですが、
    // まずは状態フラグを落とすだけにしておきます。
}
