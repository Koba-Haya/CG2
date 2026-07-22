// ============================================================
// TimelineManager.h
// 役割: タイムライン全体の再生/停止/シークを管理するシステムクラス
//       BaseSceneが所有し、シーン固有のタイムラインデータを管理する
// ============================================================
#pragma once
#include "TimelineTrack.h"
#include "ITimelineEvent.h"
#include "../../../externals/nlohmann/json.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>

// 前方宣言（Application層との結合を最小限に抑えるため、完全定義は.cppでのみインクルード）
class RailCameraComponent;

namespace AbsoluteEngine {

// 再生状態
enum class TimelinePlayState {
    Stopped,  // 停止
    Playing,  // 再生中
    Paused    // 一時停止中
};

class TimelineManager {
public:
    TimelineManager() = default;
    ~TimelineManager() = default;

    // -----------------------------------------------------------
    // 初期化・終了
    // -----------------------------------------------------------

    // タイムライン全体の長さ（秒）を設定する
    void SetDuration(float duration) { duration_ = duration; isDirty_ = true; }
    float GetDuration() const { return duration_; }

    // -----------------------------------------------------------
    // ダーティフラグ管理（未保存変更の追跡）
    // -----------------------------------------------------------

    // データが変更されたことをマークする
    void SetDirty(bool dirty = true) { isDirty_ = dirty; }

    // 未保存の変更があるか確認する
    bool IsDirty() const { return isDirty_; }

    // シーン読み込み時に RailCameraComponent を関連付ける
    // タイムラインはカメラの進行度をこのポインタ経由で同期する
    void SetRailCamera(RailCameraComponent* camera) { railCamera_ = camera; }

    // -----------------------------------------------------------
    // 再生制御
    // -----------------------------------------------------------

    // 再生開始（先頭または現在位置から）
    void Play();

    // 一時停止
    void Pause();

    // 停止（先頭に戻り、全イベントをRewind）
    void Stop();

    // 現在の再生状態を取得する
    TimelinePlayState GetPlayState() const { return playState_; }

    // -----------------------------------------------------------
    // シーク（時間ジャンプ）
    // -----------------------------------------------------------

    // 指定した時間に直接ジャンプする
    // - 前進方向: 通過したイベントをFire()
    // - 後退方向: 通過したイベントをRewind()
    // - カメラのprogress_を即座に同期する
    void Seek(float targetTime);

    // 現在の再生時間を取得する
    float GetCurrentTime() const { return currentTime_; }

    // -----------------------------------------------------------
    // 毎フレーム更新
    // -----------------------------------------------------------

    // DeltaTimeを加算して時間を進め、イベントを評価する
    // カメラのprogress_も同期して更新する
    void Update(float deltaTime);

    // -----------------------------------------------------------
    // トラック管理
    // -----------------------------------------------------------

    // トラックを追加する
    void AddTrack(std::unique_ptr<TimelineTrack> track);

    // トラックを削除する（インデックス指定）
    void RemoveTrack(size_t index);

    // トラックリストを取得する（読み取り専用）
    const std::vector<std::unique_ptr<TimelineTrack>>& GetTracks() const { return tracks_; }

    // トラックリストを取得する（書き込み可能、エディタUI用）
    std::vector<std::unique_ptr<TimelineTrack>>& GetTracksRef() { return tracks_; }

    // -----------------------------------------------------------
    // イベント選択状態管理（タスク14）
    // -----------------------------------------------------------

    // 現在選択されているイベントを取得する（非所有ポインタ）
    ITimelineEvent* GetSelectedEvent() const { return selectedEvent_; }

    // イベントを選択する（nullptr を渡すと選択解除）
    void SetSelectedEvent(ITimelineEvent* event) { selectedEvent_ = event; }

    // 選択中のイベントが削除される際に安全に nullptr にクリアする
    // RemoveTrack / RemoveEvent の前に呼ぶことでダングリングポインタを防ぐ
    void ClearSelectedEventIfMatch(ITimelineEvent* event) {
        if (selectedEvent_ == event) {
            selectedEvent_ = nullptr;
        }
    }

    // -----------------------------------------------------------
    // シリアライズ（ファイル保存/読み込み）
    // -----------------------------------------------------------

    // タイムラインデータをJSONファイルに保存する
    bool SaveToFile(const std::string& filePath) const;

    // JSONファイルからタイムラインデータを読み込む
    bool LoadFromFile(const std::string& filePath);

    // タイムラインデータをJSON文字列にシリアライズする（Undo/Redo用スナップショット）
    std::string SerializeToString() const;

    // JSON文字列からタイムラインデータを復元する（Undo/Redo用）
    // 復元後は必ず selectedEvent_ を nullptr にリセットする（ダングリングポインタ防止）
    bool LoadFromString(const std::string& jsonStr);

private:
    // カメラの進行度を現在時間から同期するヘルパー
    void SyncCameraProgress();

    // 全イベントを強制的にRewindするヘルパー（Stop時用）
    void RewindAllEvents();

private:
    // タイムライン全体の長さ（秒）
    float duration_ = 60.0f;

    // 現在の再生時間（秒）
    float currentTime_ = 0.0f;

    // 直前フレームの再生時間（スイープ判定に使用）
    float previousTime_ = 0.0f;

    // 再生状態
    TimelinePlayState playState_ = TimelinePlayState::Stopped;

    // トラックのリスト
    std::vector<std::unique_ptr<TimelineTrack>> tracks_;

    // カメラへの非所有ポインタ（BaseSceneが寿命を管理）
    RailCameraComponent* railCamera_ = nullptr;

    // 現在選択中のイベントへの非所有ポインタ（タスク14）
    // 所有権は各 TimelineTrack が持つ。削除時は ClearSelectedEventIfMatch で nullptr に戻す
    ITimelineEvent* selectedEvent_ = nullptr;

    // 未保存の変更があることを示すダーティフラグ
    // AddTrack/RemoveTrack/SetDuration 等の変更操作でtrueになり、SaveToFile後にfalseにリセットする
    // SaveToFile() は論理的にconstだが、保存完了のマーキングのため mutable にする
    mutable bool isDirty_ = false;
};

} // namespace AbsoluteEngine
