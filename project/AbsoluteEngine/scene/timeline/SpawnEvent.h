// ============================================================
// SpawnEvent.h
// 役割: プレハブIDで指定されたGameObjectをスポーンするタイムラインイベント
//       生成したオブジェクトは weak_ptr で保持し、Rewind時に安全に破棄する
// ============================================================
#pragma once
#include "ITimelineEvent.h"
#include "../../Type/Transform.h"
#include <memory>
#include <string>

namespace AbsoluteEngine {

class GameObject;

class SpawnEvent : public ITimelineEvent {
public:
    SpawnEvent() = default;
    // デストラクタ: unique_ptr による消滅時（トラック削除など）に
    // プレビューオブジェクトが確実に破棄されるよう Rewind() を呼ぶ
    ~SpawnEvent() override { Rewind(); }

    // -----------------------------------------------------------
    // 設定値（エディタUIまたはデシリアライズ時に設定される）
    // -----------------------------------------------------------

    // スポーンするプレハブのID（PrefabRegistryで解決される）
    std::string prefabId_ = "";

    // スポーン位置・回転・スケール
    Transform spawnTransform_{};

    // -----------------------------------------------------------
    // ITimelineEvent 実装
    // -----------------------------------------------------------

    std::string GetLabel() const override;

    // PrefabRegistryからパスを解決し、プレハブをスポーンして rootObjects に追加する
    // スポーンしたオブジェクトは weak_ptr で保持する
    void Fire() override;

    // weak_ptr 経由でオブジェクトの生存を確認してから安全に破棄する
    // （ダングリングポインタによるクラッシュを100%防ぐ）
    void Rewind() override;

    std::string GetEventType() const override { return "SpawnEvent"; }
    void Serialize(nlohmann::json& j) const override;
    void Deserialize(const nlohmann::json& j) override;

private:
    // Fire()でスポーンしたオブジェクトへの弱参照
    // shared_ptr を直接持つと所有権の衝突が起きるため weak_ptr を使用する
    std::weak_ptr<GameObject> spawnedObject_;
};

} // namespace AbsoluteEngine
