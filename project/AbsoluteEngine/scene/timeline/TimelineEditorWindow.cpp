// ============================================================
// TimelineEditorWindow.cpp
// ImGuiベースのタイムラインエディタUI実装
// ============================================================
#include "TimelineEditorWindow.h"
#include "PrefabRegistry.h"
#include "SpawnEvent.h"
#include "../../base/EnginePath.h"
// CommandManagerとTimelineCommandのインクルード（Undo/Redo: タスクF）
#include "../../editor/CommandManager.h"
#include <iostream>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace AbsoluteEngine {

void TimelineEditorWindow::Draw(const std::string& timelineFilePath) {
#ifdef USE_IMGUI
    if (!manager_) return;

    ImGui::Begin("Timeline Editor");

    DrawToolbar(timelineFilePath);
    ImGui::Separator();
    DrawSeekBar();
    ImGui::Separator();
    DrawTracks();

    ImGui::End();
#endif
}

#ifdef USE_IMGUI

void TimelineEditorWindow::DrawToolbar(const std::string& timelineFilePath) {
    const auto state = manager_->GetPlayState();

    // --- 再生コントロール ---
    if (state != TimelinePlayState::Playing) {
        if (ImGui::Button("Play")) {
            manager_->Play();
        }
    } else {
        if (ImGui::Button("Pause")) {
            manager_->Pause();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        manager_->Stop();
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // --- 全体尺の設定 ---
    float duration = manager_->GetDuration();
    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::DragFloat("Duration(s)", &duration, 0.5f, 1.0f, 3600.0f, "%.1f")) {
        manager_->SetDuration(duration);
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // --- ファイル操作 ---
    if (ImGui::Button("Save")) {
        if (!timelineFilePath.empty()) {
            manager_->SaveToFile(timelineFilePath);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (!timelineFilePath.empty()) {
            manager_->LoadFromFile(timelineFilePath);
        }
    }
    ImGui::SameLine();
    // PrefabRegistryを手動でリスキャンするボタン
    if (ImGui::Button("Reload Prefabs")) {
        const std::string prefabDir = EnginePath::Resolve("resources/prefabs");
        PrefabRegistry::GetInstance().ScanDirectory(prefabDir);
    }

    // --- トラック追加 ---
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputText("##TrackName", newTrackName_, sizeof(newTrackName_));
    ImGui::SameLine();
    if (ImGui::Button("+ Add Track")) {
        // Undo用に変更前のスナップショットを保存する
        const std::string beforeSnapshot = manager_->SerializeToString();
        auto track = std::make_unique<TimelineTrack>();
        track->trackName_ = newTrackName_;
        manager_->AddTrack(std::move(track));
        // 変更後のスナップショットを保存し、Undo/Redoコマンドを発行する
        if (commandManager_) {
            const std::string afterSnapshot = manager_->SerializeToString();
            auto cmd = std::make_shared<TimelineCommand>(
                // Undo: beforeSnapshotから復元する
                [mgr = manager_, beforeSnapshot]() { mgr->LoadFromString(beforeSnapshot); },
                // Redo: afterSnapshotから復元する
                [mgr = manager_, afterSnapshot]() { mgr->LoadFromString(afterSnapshot); }
            );
            commandManager_->AddCommand(cmd);
        }
    }
}

void TimelineEditorWindow::DrawSeekBar() {
    const float duration = manager_->GetDuration();
    float currentTime = manager_->GetCurrentTime();

    // 現在時刻の表示
    ImGui::Text("Time: %.2f / %.2f s", currentTime, duration);
    ImGui::SameLine();

    // シークスライダー
    ImGui::SetNextItemWidth(-1.0f); // 残り幅全体を使う
    if (ImGui::SliderFloat("##SeekBar", &currentTime, 0.0f, duration, "%.2f s")) {
        // スライダーが動いたらシークを実行する
        // （ドラッグ中も毎フレーム呼ばれるが、スイープ判定で正しく処理される）
        manager_->Seek(currentTime);
    }
}

void TimelineEditorWindow::DrawTracks() {
    const auto& tracks = manager_->GetTracks();
    const float duration = manager_->GetDuration();
    const float currentTime = manager_->GetCurrentTime();

    if (tracks.empty()) {
        ImGui::TextDisabled("トラックがありません。[+ Add Track]で追加してください。");
        return;
    }

    // タイムラインのタイムスケール（ピクセル/秒）
    const float pixelsPerSecond = kTimelineAreaWidth / duration;

    for (size_t i = 0; i < tracks.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));

        auto& track = manager_->GetTracksRef()[i];
        const float trackLabelWidth = 120.0f;

        // --- トラック名 ---
        ImGui::Text("[%zu] %s", i, track->trackName_.c_str());
        ImGui::SameLine(trackLabelWidth);

        // --- トラック削除ボタン ---
        if (ImGui::SmallButton("×")) {
            // Undo用に変更前のスナップショットを保存する
            const std::string beforeSnapshot = manager_->SerializeToString();
            manager_->RemoveTrack(i);
            // Undo/Redoコマンドを発行する
            if (commandManager_) {
                const std::string afterSnapshot = manager_->SerializeToString();
                auto cmd = std::make_shared<TimelineCommand>(
                    [mgr = manager_, beforeSnapshot]() { mgr->LoadFromString(beforeSnapshot); },
                    [mgr = manager_, afterSnapshot]() { mgr->LoadFromString(afterSnapshot); }
                );
                commandManager_->AddCommand(cmd);
            }
            ImGui::PopID();
            break; // イテレータが無効になるのでbreak
        }
        ImGui::SameLine();

        // --- イベント追加ボタン ---
        if (ImGui::SmallButton("+ Event")) {
            addEventTargetTrackIndex_ = static_cast<int>(i);
            addEventTime_ = currentTime; // 現在位置にデフォルト設定
            selectedPrefabIndex_ = 0;
            ImGui::OpenPopup("AddEventPopup");
        }

        // ポップアップはOpenPopupと同じIDスタック（PushID内）で描画する必要がある
        DrawAddEventPopup();

        // --- イベントレーン（クリック可能な帯） ---
        DrawTrackLane(i);

        ImGui::PopID();
    }
}

void TimelineEditorWindow::DrawTrackLane(size_t trackIndex) {
    auto& tracks = manager_->GetTracksRef();
    if (trackIndex >= tracks.size()) return;

    auto& track = tracks[trackIndex];
    const float duration = manager_->GetDuration();
    const float pixelsPerSecond = kTimelineAreaWidth / duration;

    // トラックの背景バー
    const ImVec2 laneStart = ImGui::GetCursorScreenPos();
    const float trackLabelWidth = 120.0f;
    const ImVec2 laneBgMin = { laneStart.x, laneStart.y };
    const ImVec2 laneBgMax = { laneStart.x + kTimelineAreaWidth, laneStart.y + kTrackHeight };

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(laneBgMin, laneBgMax, IM_COL32(50, 50, 50, 200));
    drawList->AddRect(laneBgMin, laneBgMax, IM_COL32(100, 100, 100, 255));

    // 現在時刻のカーソルライン
    const float currentTime = manager_->GetCurrentTime();
    const float cursorX = laneBgMin.x + currentTime * pixelsPerSecond;
    drawList->AddLine({ cursorX, laneBgMin.y }, { cursorX, laneBgMax.y }, IM_COL32(255, 220, 0, 255), 2.0f);

    // 各イベントをひし形（Diamond）でレーンに描画する（タスク13）
    const auto& events = track->GetEvents();
    size_t removeIndex = SIZE_MAX;
    const float kDiamondHalfW = 9.0f;  // ひし形の半幅（水平方向）
    const float kDiamondHalfH = 10.0f; // ひし形の半高（垂直方向）

    for (size_t j = 0; j < events.size(); ++j) {
        const auto& event = events[j];
        const float eventX = laneBgMin.x + event->triggerTime_ * pixelsPerSecond;
        const float centerY = laneBgMin.y + kTrackHeight * 0.5f;

        // ひし形の4頂点（上・右・下・左）を計算する
        const ImVec2 ptTop   = { eventX,                centerY - kDiamondHalfH };
        const ImVec2 ptRight = { eventX + kDiamondHalfW, centerY };
        const ImVec2 ptBot   = { eventX,                centerY + kDiamondHalfH };
        const ImVec2 ptLeft  = { eventX - kDiamondHalfW, centerY };

        // 選択状態に応じて色を変える
        const bool isSelected = (manager_->GetSelectedEvent() == event.get());
        const ImU32 fillColor = isSelected
            ? IM_COL32(255, 180, 50, 240)   // 選択中: ゴールド
            : IM_COL32(80, 200, 120, 230);  // 通常:   グリーン
        const ImU32 outlineColor = isSelected
            ? IM_COL32(255, 120, 0, 255)    // 選択中: オレンジ枠
            : IM_COL32(40, 160, 80, 255);   // 通常:   ダークグリーン枠

        // ひし形を塗りつぶしと枠線で描画する
        ImVec2 pts[4] = { ptTop, ptRight, ptBot, ptLeft };
        drawList->AddConvexPolyFilled(pts, 4, fillColor);
        drawList->AddPolyline(pts, 4, outlineColor, ImDrawFlags_Closed, 1.5f);

        // イベント名をひし形の右側に表示する（フォントが小さめなので横に出す）
        drawList->AddText({ eventX + kDiamondHalfW + 2.0f, centerY - 6.0f },
            IM_COL32(220, 220, 220, 255), event->GetLabel().c_str());

        // ひし形のAABB領域をImGuiのヒット領域とする（InvisibleButton）
        const ImVec2 evMin = { eventX - kDiamondHalfW, centerY - kDiamondHalfH };
        const ImVec2 evMax = { eventX + kDiamondHalfW, centerY + kDiamondHalfH };
        ImGui::SetCursorScreenPos(evMin);
        ImGui::InvisibleButton(("ev_" + std::to_string(trackIndex) + "_" + std::to_string(j)).c_str(),
            { evMax.x - evMin.x, evMax.y - evMin.y });

        // 左クリックでイベントを選択する（タスク13: selectedEvent_に通知）
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            manager_->SetSelectedEvent(event.get());
        }

        // 右クリックで削除コンテキストメニューを表示する
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("EventCtx_" + std::to_string(trackIndex) + "_" + std::to_string(j)).c_str());
        }
        if (ImGui::BeginPopup(("EventCtx_" + std::to_string(trackIndex) + "_" + std::to_string(j)).c_str())) {
            ImGui::Text("%s @ %.2fs", event->GetLabel().c_str(), event->triggerTime_);
            ImGui::Separator();
            if (ImGui::MenuItem("削除")) {
                // 削除前に選択状態をクリアする（ダングリングポインタ防止: タスク14）
                manager_->ClearSelectedEventIfMatch(event.get());
                removeIndex = j;
            }
            ImGui::EndPopup();
        }
    }

    // 削除が要求されたイベントを処理する（ループ外でイテレータ無効化を防ぐ）
    if (removeIndex != SIZE_MAX) {
        track->RemoveEvent(removeIndex);
    }

    // InvisibleButton でレーン全体を ImGui のヒット領域にする
    ImGui::SetCursorScreenPos(laneBgMin);
    ImGui::InvisibleButton(("lane_" + std::to_string(trackIndex)).c_str(),
        { kTimelineAreaWidth, kTrackHeight });

    ImGui::Dummy({ 0, 4 }); // トラック間のスペース
}


void TimelineEditorWindow::DrawAddEventPopup() {
    if (!ImGui::BeginPopup("AddEventPopup")) return;

    ImGui::Text("新規イベントを追加");
    ImGui::Separator();

    // 発火時刻
    ImGui::InputFloat("発火時刻 (s)", &addEventTime_, 0.1f, 1.0f, "%.2f");

    // プレハブID選択（PrefabRegistryから一覧を取得）
    const std::vector<std::string> prefabIds = PrefabRegistry::GetInstance().GetAllIds();

    if (prefabIds.empty()) {
        ImGui::TextColored({ 1.0f, 0.5f, 0.5f, 1.0f },
            "プレハブが登録されていません。[Reload Prefabs]を押してください。");
    } else {
        // ドロップダウンリスト
        if (selectedPrefabIndex_ >= static_cast<int>(prefabIds.size())) {
            selectedPrefabIndex_ = 0;
        }

        // ImGui::Combo に渡す用のCスタイル文字列配列を生成する
        std::vector<const char*> idCStrs;
        idCStrs.reserve(prefabIds.size());
        for (const auto& id : prefabIds) {
            idCStrs.push_back(id.c_str());
        }

        ImGui::Combo("プレハブID", &selectedPrefabIndex_,
            idCStrs.data(), static_cast<int>(idCStrs.size()));
    }

    ImGui::Separator();

    // 追加ボタン
    if (!prefabIds.empty() && ImGui::Button("追加")) {
        if (addEventTargetTrackIndex_ >= 0 &&
            addEventTargetTrackIndex_ < static_cast<int>(manager_->GetTracks().size())) {

            auto event = std::make_unique<SpawnEvent>();
            event->triggerTime_ = addEventTime_;
            event->prefabId_ = prefabIds[static_cast<size_t>(selectedPrefabIndex_)];

            manager_->GetTracksRef()[static_cast<size_t>(addEventTargetTrackIndex_)]
                ->AddEvent(std::move(event));
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("キャンセル")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

#endif // USE_IMGUI

} // namespace AbsoluteEngine
