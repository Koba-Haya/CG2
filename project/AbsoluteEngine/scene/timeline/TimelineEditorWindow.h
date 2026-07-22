// ============================================================
// TimelineEditorWindow.h
// 役割: ImGuiを使ってタイムラインを可視化・編集するエディタウィンドウ
//       コアロジック（TimelineManager）とUIの分離を徹底する
// ============================================================
#pragma once
#include "TimelineManager.h"
#include <string>

// 前方宣言（CommandManagerの完全定義は.cppでのみインクルードする）
namespace AbsoluteEngine { class CommandManager; }

namespace AbsoluteEngine {

class TimelineEditorWindow {
public:
    TimelineEditorWindow() = default;
    ~TimelineEditorWindow() = default;

    // TimelineManager への参照を設定する
    // TimelineEditorWindowはManagerの寿命を管理しない（非所有参照）
    void SetManager(TimelineManager* manager) { manager_ = manager; }

    // CommandManager への参照を設定する（Undo/Redo用: タスクF）
    // nullptr を渡した場合はコマンド発行を無効化する
    void SetCommandManager(CommandManager* commandManager) { commandManager_ = commandManager; }

    // 毎フレームUIを描画する（BaseScene::DrawEditorUI から呼ばれる想定）
    // USE_IMGUI マクロが無効な場合は何もしない
    void Draw(const std::string& timelineFilePath);

private:
#ifdef USE_IMGUI
    // ツールバー（Play/Pause/Stop/Save/Load ボタン）を描画する
    void DrawToolbar(const std::string& timelineFilePath);

    // シークバーを描画する
    void DrawSeekBar();

    // トラックとイベントのレーンを描画する
    void DrawTracks();

    // 1つのトラックのイベントレーンを描画する（インデックス指定）
    void DrawTrackLane(size_t trackIndex);

    // イベント追加ポップアップを描画する（プレハブID選択付き）
    void DrawAddEventPopup();
#endif

private:
    // TimelineManager への非所有ポインタ
    TimelineManager* manager_ = nullptr;

    // CommandManager への非所有ポインタ（Undo/Redo用: タスクF）
    // nullptr の場合はコマンド発行を行わない
    CommandManager* commandManager_ = nullptr;

#ifdef USE_IMGUI
    // UIの状態変数（エディタのみで使用）

    // トラック追加時の仮名
    char newTrackName_[128] = "NewTrack";

    // イベント追加ポップアップの対象トラックインデックス
    int addEventTargetTrackIndex_ = -1;

    // 追加するイベントの発火時刻
    float addEventTime_ = 0.0f;

    // 選択中のプレハブIDのインデックス（ドロップダウン用）
    int selectedPrefabIndex_ = 0;

    // タイムライン表示エリアのピクセル幅
    static constexpr float kTimelineAreaWidth = 800.0f;

    // トラック1本の高さ（ピクセル）
    static constexpr float kTrackHeight = 30.0f;
#endif
};

} // namespace AbsoluteEngine
