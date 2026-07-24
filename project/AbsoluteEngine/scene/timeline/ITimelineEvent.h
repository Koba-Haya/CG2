// ============================================================
// ITimelineEvent.h
// 役割: タイムライン上のすべてのイベントが実装する純粋インターフェース
//       Fire/Rewindパターンによりシーク時の状態不整合を防ぐ
// ============================================================
#pragma once
#include "../../../externals/nlohmann/json.hpp"

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
};

} // namespace AbsoluteEngine
