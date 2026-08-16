// ============================================================
// ITimelineEvent.h
// 役割: タイムライン上のすべてのイベントが実装する純粋インターフェース
//       Fire/Rewindパターンによりシーク時の状態不整合を防ぐ
// ============================================================
#pragma once
#include "../../../externals/nlohmann/json.hpp"
#include "../../Type/Transform.h"

namespace AbsoluteEngine {

class ITimelineEvent {
public:
    virtual ~ITimelineEvent() = default;

    // -----------------------------------------------------------
    // 必須プロパティ
    // -----------------------------------------------------------

    // イベントが発火する時刻（秒）
    float triggerTime_ = 0.0f;

    // 発火済みフラグ（状態ベースのイベント管理に使用）
    // true: Fire()済み / false: まだ発火していない or Rewind()済み
    bool isFired_ = false;

    // エディタ上での表示名（デバッグ・UI用）
    virtual std::string GetLabel() const = 0;

    // -----------------------------------------------------------
    // コアメソッド（Fire/Rewindパターン）
    // -----------------------------------------------------------

    // 発火: タイムラインが triggerTime_ を前進方向に通過したときに呼ばれる
    virtual void Fire() = 0;

    // 巻き戻し: タイムラインが triggerTime_ を後退方向に通過したとき（シーク時など）に呼ばれる
    // 実装側は Fire() で生成したオブジェクト等を安全に破棄しなければならない
    virtual void Rewind() = 0;

    // -----------------------------------------------------------
    // シリアライズ
    // -----------------------------------------------------------

    // イベントの種別識別子（JSON保存時のタグ）
    virtual std::string GetEventType() const = 0;

    // イベントのデータをJSONに書き出す（triggerTime_ はTimelineTrack側で管理）
    virtual void Serialize(nlohmann::json& j) const = 0;

    // JSONからイベントのデータを復元する
    virtual void Deserialize(const nlohmann::json& j) = 0;

    // -----------------------------------------------------------
    // エディタインスペクタ用UI（USE_IMGUI無効時は各実装側で空実装にする）
    // -----------------------------------------------------------
    // イベント種別ごとの固有プロパティを描画する。IComponent::DrawInspectorUI()と
    // 同じ考え方で、呼び出し側（エディタ）が具象クラスを知らなくても
    // ポリモーフィックにUIを出せるようにする。
    // activated/deactivatedAfterEdit は、描画した各ウィジェットの直後で
    // ImGui::IsItemActivated() / IsItemDeactivatedAfterEdit() をOR集計して
    // 呼び出し元に伝えるための出力引数（Undo用スナップショットのタイミング判定に使う）。
    virtual void DrawInspectorUI(bool& activated, bool& deactivatedAfterEdit) {}

    // -----------------------------------------------------------
    // スポーン位置の自動配置（エディタの「追加」操作から呼ばれる）
    // -----------------------------------------------------------
    // 新規追加時に、その発火時刻におけるレールカメラ前方のアンカー座標
    // （TimelineManager::ComputeSpawnAnchorTransform()の結果）を渡す。
    // スポーン系イベント（SpawnEvent/FormationSpawnEvent等）はこれをオーバーライドして
    // 自身の位置フィールドに反映する。エディタ側は具象クラスの位置フィールド名を
    // 知らなくてよい（DrawInspectorUIと同じ考え方のポリモーフィックなフック）。
    virtual void SetSpawnAnchorTransform(const Transform& t) {}
};

} // namespace AbsoluteEngine
