// ============================================================
// TimelineEventFactory.h
// 役割: イベント種別名からITimelineEventの具象インスタンスを生成するファクトリ
//       ComponentFactory と同じパターン。エンジン層はSpawnEventのみを
//       組み込みで知っており、Application層固有のイベント種別（FormationSpawnEvent等）は
//       GameScene::Initialize() 等から実行時にRegister()される想定。
//       これにより TimelineTrack/エディタ が Application 層のヘッダに依存せずに
//       ゲーム固有のタイムラインイベントを生成・編集できる。
// ============================================================
#pragma once
#include "ITimelineEvent.h"
#include "SpawnEvent.h"
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>

namespace AbsoluteEngine {

class TimelineEventFactory {
public:
    using CreatorFunc = std::function<std::unique_ptr<ITimelineEvent>()>;

    static TimelineEventFactory& GetInstance() {
        static TimelineEventFactory instance;
        return instance;
    }

    void Register(const std::string& name, CreatorFunc func) {
        creators_[name] = func;
    }

    std::unique_ptr<ITimelineEvent> Create(const std::string& name) {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second();
        }
        return nullptr;
    }

    std::vector<std::string> GetRegisteredEventNames() const {
        std::vector<std::string> names;
        for (const auto& pair : creators_) {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    TimelineEventFactory() {
        // エンジン組み込みのイベント種別を登録
        Register("SpawnEvent", []() { return std::make_unique<SpawnEvent>(); });
    }
    std::map<std::string, CreatorFunc> creators_;
};

} // namespace AbsoluteEngine
