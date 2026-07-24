// ============================================================
// TimelineEditorWindow.h
// 役割: ImGuiを使ってタイムラインを可視化・編集するエディタウィンドウ
//       コアロジック（TimelineManager）とUIの分離を徹底する
// ============================================================
#pragma once
#include "TimelineManager.h"
#include <string>
#include <functional>

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

    // Debug/Game カメラ切替フラグへの非所有ポインタを設定する
    // Application層（GameScene等）が持つ bool を直接トグルするための橋渡し
    // nullptr の場合はトグルボタンを表示しない
    void SetDebugCameraFlag(bool* flag) { debugCameraFlag_ = flag; }

    // トラック/イベントの追加・削除等でシーンが変更された際に呼ばれるコールバックを設定する
    // 既存のオートセーブ経路（EditorUIManager::SetSceneModified）に接続するために使う
    void SetOnModifiedCallback(std::function<void()> callback) { onModified_ = std::move(callback); }

    // 毎フレームUIを描画する（BaseScene::DrawEditorUI から呼ばれる想定）
    // USE_IMGUI マクロが無効な場合は何もしない
    void Draw(const std::string& timelineFilePath);

private:
#ifdef USE_IMGUI
    // ツールバー（1行目: 再生系+Debug Cameraトグル、2行目: プレハブ関連）を描画する
    void DrawToolbar(const std::string& timelineFilePath);

    // ルーラー（目盛り+再生ヘッド）とシークスライダーを描画する
    // トラックレーンと同じ座標系（ラベル列オフセット+可変レーン幅）を共有する
    void DrawSeekBar();

    // トラックとイベントのレーンを描画する
    void DrawTracks();

    // 1つのトラックのイベントレーンを描画する（インデックスとレーン幅を指定）
    void DrawTrackLane(size_t trackIndex, float laneWidth);

    // イベント追加ポップアップを描画する（プレハブID選択付き）
    void DrawAddEventPopup();

    // トラックインデックスに応じた固定パレット色を返す（重なり識別用、戻り値はImU32と同一表現）
    static unsigned int GetTrackColor(size_t trackIndex);
#endif

private:
    // TimelineManager への非所有ポインタ
    TimelineManager* manager_ = nullptr;

    // CommandManager への非所有ポインタ（Undo/Redo用: タスクF）
    // nullptr の場合はコマンド発行を行わない
    CommandManager* commandManager_ = nullptr;

    // Debug/Game カメラ切替フラグへの非所有ポインタ
    bool* debugCameraFlag_ = nullptr;

    // シーン変更通知コールバック（オートセーブ用）
    std::function<void()> onModified_;

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

    // トラックのラベル列（名前+ボタン）の固定幅（ピクセル）
    // ルーラー・シークバー・各トラックレーンはこの幅ぶんオフセットして座標系を揃える
    static constexpr float kTrackLabelWidth = 130.0f;

    // トラック1本の高さ（ピクセル）
    static constexpr float kTrackHeight = 30.0f;
#endif
};

} // namespace AbsoluteEngine
