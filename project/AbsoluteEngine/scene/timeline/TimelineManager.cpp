// ============================================================
// TimelineManager.cpp
// タイムライン再生・シーク・シリアライズの実装
// ============================================================
#include "TimelineManager.h"
// RailCameraComponent はApplication層にあるため完全定義をインクルード
#include "../../../Application/camera/RailCameraComponent.h"
#include "../../base/EnginePath.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace AbsoluteEngine {

// -----------------------------------------------------------
// 再生制御
// -----------------------------------------------------------

void TimelineManager::Play() {
    if (playState_ == TimelinePlayState::Playing) return;

    // 停止状態・一時停止状態のどちらからでも、現在の currentTime_ の位置から再生を再開する
    // （= Play From Current Time の実現）
    // ※ previousTime_ も currentTime_ に合わせてから開始することで、
    //    再開直後フレームのスイープ判定が空振りするのを防ぐ
    previousTime_ = currentTime_;

    playState_ = TimelinePlayState::Playing;
}

void TimelineManager::Pause() {
    if (playState_ != TimelinePlayState::Playing) return;
    playState_ = TimelinePlayState::Paused;
}

void TimelineManager::Stop() {
    playState_ = TimelinePlayState::Stopped;

    // 全イベントをRewindして状態をクリーンにする
    RewindAllEvents();

    // 時間を先頭にリセット
    previousTime_ = 0.0f;
    currentTime_ = 0.0f;

    // カメラも先頭に同期する
    SyncCameraProgress();
}

// -----------------------------------------------------------
// シーク（タスク4: カメラ・プレイヤーのシーク同期を含む）
// -----------------------------------------------------------

void TimelineManager::Seek(float targetTime) {
    // 有効範囲にクランプ
    targetTime = std::clamp(targetTime, 0.0f, duration_);

    // スイープ判定のために以前の時間を保存する
    previousTime_ = currentTime_;
    currentTime_ = targetTime;

    // 各トラックにスイープ判定を実行させる（Forward/Rewindを自動判断）
    for (auto& track : tracks_) {
        track->Update(currentTime_, previousTime_);
    }

    // カメラの進行度を即時同期する（Lerpによる遅延ワープを防ぐため直接設定）
    SyncCameraProgress();
}

// -----------------------------------------------------------
// 毎フレーム更新
// -----------------------------------------------------------

void TimelineManager::Update(float deltaTime) {
    if (playState_ != TimelinePlayState::Playing) return;

    // 前フレームの時間を記録してからDeltaTimeを加算する
    previousTime_ = currentTime_;
    currentTime_ += deltaTime;

    // タイムラインの終端を超えたら停止する
    if (currentTime_ >= duration_) {
        currentTime_ = duration_;

        // 末尾のイベントを発火させてから停止する
        for (auto& track : tracks_) {
            track->Update(currentTime_, previousTime_);
        }

        // 末端到達で自動停止
        Stop();
        return;
    }

    // 各トラックを更新してスイープ判定を実行する
    for (auto& track : tracks_) {
        track->Update(currentTime_, previousTime_);
    }

    // カメラの進行度を同期する
    SyncCameraProgress();
}

// -----------------------------------------------------------
// トラック管理
// -----------------------------------------------------------

void TimelineManager::AddTrack(std::unique_ptr<TimelineTrack> track) {
    tracks_.push_back(std::move(track));
    // トラック追加はデータ変更なのでダーティフラグを立てる
    isDirty_ = true;
}

void TimelineManager::RemoveTrack(size_t index) {
    if (index >= tracks_.size()) return;

    // 削除前にトラック内の全イベントをRewindし、
    // 選択中イベントがこのトラックに含まれている場合はnullptrにクリアする（タスク14）
    const auto& events = tracks_[index]->GetEvents();
    for (const auto& ev : events) {
        ClearSelectedEventIfMatch(ev.get());
    }
    tracks_[index]->RewindAll();
    tracks_.erase(tracks_.begin() + static_cast<ptrdiff_t>(index));
    // トラック削除はデータ変更なのでダーティフラグを立てる
    isDirty_ = true;
}

// -----------------------------------------------------------
// シリアライズ
// -----------------------------------------------------------

bool TimelineManager::SaveToFile(const std::string& filePath) const {
    nlohmann::json j;

    j["duration"] = duration_;

    // JSONの階層構造: { "tracks": [ { "trackName": "...", "events": [...] } ] }
    // この構造を保つことで将来の拡張（オーディオトラックなど）が容易になる
    nlohmann::json tracksJson = nlohmann::json::array();
    for (const auto& track : tracks_) {
        nlohmann::json trackJson;
        track->Serialize(trackJson);
        tracksJson.push_back(trackJson);
    }
    j["tracks"] = tracksJson;

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[TimelineManager] ファイルの書き込みに失敗しました: " << filePath << std::endl;
        return false;
    }

    file << j.dump(4);
    // 保存完了後はダーティフラグをクリアする
    isDirty_ = false;
    std::cout << "[TimelineManager] タイムラインデータを保存しました: " << filePath << std::endl;
    return true;
}

bool TimelineManager::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[TimelineManager] ファイルが見つかりません: " << filePath << std::endl;
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[TimelineManager] JSONパースエラー: " << e.what() << std::endl;
        return false;
    }

    duration_ = j.value("duration", 60.0f);
    tracks_.clear();

    if (j.contains("tracks") && j["tracks"].is_array()) {
        for (const auto& trackJson : j["tracks"]) {
            auto track = std::make_unique<TimelineTrack>();
            track->Deserialize(trackJson);
            tracks_.push_back(std::move(track));
        }
    }

    // 読み込み後は先頭にリセットする
    currentTime_ = 0.0f;
    previousTime_ = 0.0f;
    playState_ = TimelinePlayState::Stopped;

    std::cout << "[TimelineManager] タイムラインデータを読み込みました: " << filePath
              << " (トラック数: " << tracks_.size() << ")" << std::endl;
    return true;
}

// -----------------------------------------------------------
// プライベートヘルパー
// -----------------------------------------------------------

void TimelineManager::SyncCameraProgress() {
    if (!railCamera_) return;
    if (duration_ <= 0.0f) return;

    // 現在時間の全体に対する割合を計算してカメラに直接設定する
    // クランプして 0.0～1.0 の範囲に収める
    const float progress = std::clamp(currentTime_ / duration_, 0.0f, 1.0f);
    railCamera_->SetProgress(progress);
}

void TimelineManager::RewindAllEvents() {
    for (auto& track : tracks_) {
        track->RewindAll();
    }
}

// -----------------------------------------------------------
// 文字列ベースのシリアライズ（Undo/Redo用スナップショット）
// -----------------------------------------------------------

std::string TimelineManager::SerializeToString() const {
    nlohmann::json j;
    j["duration"] = duration_;

    nlohmann::json tracksJson = nlohmann::json::array();
    for (const auto& track : tracks_) {
        nlohmann::json trackJson;
        track->Serialize(trackJson);
        tracksJson.push_back(trackJson);
    }
    j["tracks"] = tracksJson;

    return j.dump();
}

bool TimelineManager::LoadFromString(const std::string& jsonStr) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(jsonStr);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[TimelineManager] JSON文字列のパースエラー: " << e.what() << std::endl;
        return false;
    }

    // 現在のイベントを全てRewindしてから再構築する
    RewindAllEvents();

    duration_ = j.value("duration", 60.0f);
    tracks_.clear();

    if (j.contains("tracks") && j["tracks"].is_array()) {
        for (const auto& trackJson : j["tracks"]) {
            auto track = std::make_unique<TimelineTrack>();
            track->Deserialize(trackJson);
            tracks_.push_back(std::move(track));
        }
    }

    // 復元後は先頭にリセットする
    currentTime_ = 0.0f;
    previousTime_ = 0.0f;
    playState_ = TimelinePlayState::Stopped;

    // ダングリングポインタ防止: 復元後は選択中イベントをクリアする
    selectedEvent_ = nullptr;

    std::cout << "[TimelineManager] JSON文字列からタイムラインデータを復元しました"
              << " (トラック数: " << tracks_.size() << ")" << std::endl;
    return true;
}

} // namespace AbsoluteEngine
