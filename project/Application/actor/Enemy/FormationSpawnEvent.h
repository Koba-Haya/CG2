// ============================================================
// FormationSpawnEvent.h
// 役割: 同一プレハブを複数体（3〜5体）まとめてクラスタ状にスポーンする
//       タイムラインイベント。SpawnEventと違い1イベントで編隊全体を生成する。
//       生成した各メンバーには StraightMoveComponent（移動）と
//       FormationMemberComponent（GameScene側の編隊全滅ボーナス判定用）を付与する。
//
//       StraightMoveComponent/FormationMemberComponentはApplication（ゲーム）層の
//       コンポーネントのため、このクラス自身もApplication層に置く。
//       AbsoluteEngine（エンジン）層のTimelineTrack/エディタからは
//       TimelineEventFactory経由で名前だけで生成・利用され、具象型には依存しない。
// ============================================================
#pragma once
#include "AbsoluteEngine/scene/timeline/ITimelineEvent.h"
#include "Type/Transform.h"
#include <memory>
#include <string>
#include <vector>

namespace AbsoluteEngine { class GameObject; }

class FormationSpawnEvent : public AbsoluteEngine::ITimelineEvent {
public:
    FormationSpawnEvent() = default;
    ~FormationSpawnEvent() override { Rewind(); }

    // -----------------------------------------------------------
    // 設定値（エディタUIまたはデシリアライズ時に設定される）
    // -----------------------------------------------------------

    // スポーンするプレハブのID（PrefabRegistryで解決される。編隊メンバー全員が同一プレハブ）
    std::string prefabId_ = "";

    // 編隊の中心位置・回転・スケール（各メンバーはこの位置を基準にXY平面でオフセットされる）
    Transform basePosition_{};

    // 編隊のメンバー数（3〜5にクランプ）
    int memberCount_ = 4;

    // クラスタ配置のばらつき半径
    float clusterRadius_ = 3.0f;

    // -----------------------------------------------------------
    // ITimelineEvent 実装
    // -----------------------------------------------------------

    std::string GetLabel() const override;

    // memberCount_ 体をbasePosition_周辺にランダム配置し、rootObjects に追加する
    void Fire() override;

    // Fire()でスポーンした全メンバーを安全に破棄する
    void Rewind() override;

    std::string GetEventType() const override { return "FormationSpawnEvent"; }
    void Serialize(nlohmann::json& j) const override;
    void Deserialize(const nlohmann::json& j) override;

    void DrawInspectorUI(bool& activated, bool& deactivatedAfterEdit) override;
    void SetSpawnAnchorTransform(const Transform& t) override { basePosition_ = t; }

private:
    // Fire()でスポーンした各メンバーへの弱参照
    std::vector<std::weak_ptr<AbsoluteEngine::GameObject>> spawnedObjects_;
};
