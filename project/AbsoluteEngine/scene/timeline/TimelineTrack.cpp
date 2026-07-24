// ============================================================
// TimelineTrack.cpp
// イベントコンテナとスイープ判定ロジックの実装
// ============================================================
#include "TimelineTrack.h"
#include "SpawnEvent.h"
#include <algorithm>
#include <iostream>

namespace AbsoluteEngine {

void TimelineTrack::AddEvent(std::unique_ptr<ITimelineEvent> event) {
    events_.push_back(std::move(event));
    SortEvents();
}

void TimelineTrack::RemoveEvent(size_t index) {
    if (index >= events_.size()) return;

    // 削除前にRewindして状態をクリーンにする
    events_[index]->Rewind();
    events_.erase(events_.begin() + static_cast<ptrdiff_t>(index));
}

void TimelineTrack::SortEvents() {
    std::sort(events_.begin(), events_.end(),
        [](const std::unique_ptr<ITimelineEvent>& a, const std::unique_ptr<ITimelineEvent>& b) {
            return a->triggerTime_ < b->triggerTime_;
        });
}

void TimelineTrack::Update(float currentTime, float previousTime) {
    if (events_.empty()) return;

    const bool isForward = (currentTime >= previousTime); // 前進方向かどうか

    if (isForward) {
        // 前進方向: triggerTime_ が currentTime 以下かつ未発火のイベントを Fire()
        // 区間判定ではなく isFired_ フラグで管理するため、シークで飛ばした場合も正確に発火する
        for (auto& event : events_) {
            if (event->triggerTime_ <= currentTime && !event->isFired_) {
                event->Fire();
                event->isFired_ = true;
            }
        }
    } else {
        // 後退方向（シーク巻き戻し）: triggerTime_ が currentTime を超えかつ発火済みのイベントを Rewind()
        // ガードレール: 逆順でRewindすることで依存関係のある場合も安全に処理できる
        for (auto it = events_.rbegin(); it != events_.rend(); ++it) {
            if ((*it)->triggerTime_ > currentTime && (*it)->isFired_) {
                (*it)->Rewind();
                (*it)->isFired_ = false;
            }
        }
    }
}

void TimelineTrack::RewindAll() {
    // 逆順でRewindして依存関係の問題を防ぐ
    for (auto it = events_.rbegin(); it != events_.rend(); ++it) {
        (*it)->Rewind();
        // 発火済みフラグもリセットして次の再生に備える
        (*it)->isFired_ = false;
    }
}

void TimelineTrack::Serialize(nlohmann::json& j) const {
    j["trackName"] = trackName_;

    nlohmann::json eventsJson = nlohmann::json::array();
    for (const auto& event : events_) {
        nlohmann::json eventJson;
        // イベントの種別を保存（デシリアライズ時に適切なクラスを選択するため）
        eventJson["eventType"]   = event->GetEventType();
        eventJson["triggerTime"] = event->triggerTime_;
        event->Serialize(eventJson);
        eventsJson.push_back(eventJson);
    }
    j["events"] = eventsJson;
}

void TimelineTrack::Deserialize(const nlohmann::json& j) {
    trackName_ = j.value("trackName", "NewTrack");
    events_.clear();

    if (!j.contains("events") || !j["events"].is_array()) return;

    for (const auto& eventJson : j["events"]) {
        const std::string eventType = eventJson.value("eventType", "");

        // イベント種別に応じてインスタンスを生成する
        // 将来の拡張時はここに分岐を追加するだけでよい
        std::unique_ptr<ITimelineEvent> event;

        if (eventType == "SpawnEvent") {
            event = std::make_unique<SpawnEvent>();
        }
        // 将来の拡張例:
        // else if (eventType == "AudioEvent") { event = std::make_unique<AudioEvent>(); }

        if (event) {
            event->triggerTime_ = eventJson.value("triggerTime", 0.0f);
            event->Deserialize(eventJson);
            events_.push_back(std::move(event));
        } else {
            std::cerr << "[TimelineTrack] 未知のイベント種別: \"" << eventType << "\"" << std::endl;
        }
    }

    // デシリアライズ後はソートして時間順を保証する
    SortEvents();
}

} // namespace AbsoluteEngine
