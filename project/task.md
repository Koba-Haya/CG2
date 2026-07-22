# タイムラインエディタ 実装タスクリスト（Claudeへの指示書）

あなた（Claude）の役割は【超一流プログラマー・実装専任】です。
以下のタスクを1つずつ確実に、既存コードを壊さずに実装してください。
※ファイル編集の際は必ず**【UTF-8 (BOM付き)】**で保存すること。
※最小コンテキストの維持を徹底し、無駄なファイル探索を行わないこと。

## 進行状況
- `[ ]` 未完了
- `[/]` 進行中
- `[x]` 完了

---
## 【フェーズ1：基盤データの修正とマスター化】

- `[x]` **タスクA: 文字コード修正と状態ベースのイベント管理**
  - **対象ファイル**: `ITimelineEvent.h`, `TimelineTrack.cpp`, `SpawnEvent.h`, `SpawnEvent.cpp`
  - **実装内容**:
    1. ~~上記関連ファイルをすべて **UTF-8 (BOM付き)** に再保存し、UIの文字化けを直す。~~
    2. `ITimelineEvent` に `bool isFired_ = false;` を追加 ✅
    3. `TimelineTrack::Update` の判定を状態ベースに書き換え ✅
       - Forward時: `if (event->triggerTime_ <= currentTime && !event->isFired_) { Fire(); isFired_ = true; }`
       - Rewind時: `if (event->triggerTime_ > currentTime && event->isFired_) { Rewind(); isFired_ = false; }`
    4. `SpawnEvent` にデストラクタ `~SpawnEvent() override { Rewind(); }` を追加 ✅

- `[x]` **タスクB: タイムラインとシーンの二重管理廃止（強制スナップ）**
  - **対象ファイル**: `TimelineManager.cpp/.h`, `BaseScene.cpp`
  - **実装内容**:
    1. `TimelineManager` に `bool isDirty_ = false;` フラグを追加・変更時trueに設定 ✅
    2. `TimelineManager::Stop()` 時に全イベントの `isFired_ = false` をリセット ✅（RewindAll経由）
    3. `BaseScene::SaveEditorScene()` 内で `timelineManager_.SaveToFile()` を同時呼び出し ✅
    4. `BaseScene::LoadTimeline()` 直後に `timelineManager_.Stop()` を呼んで強制スナップ ✅

---
## 【フェーズ2：疎結合フックとエディタ仕様の刷新】

- `[x]` **タスクC: 敵撃破コールバックの依存性注入 (DI)**
  - **対象ファイル**: `BaseScene.h`, `GameScene.h`, `GameScene.cpp`
  - **実装内容**:
    1. `BaseScene::AddRootObject` を `virtual` 関数にする。✅
    2. `GameScene` 側で `AddRootObject` をオーバーライドする。✅
    3. オーバーライド内で `BaseScene::AddRootObject(obj);` を呼んだ後、`obj` が `EnemyComponent` を持っていれば、`onDestroyed` を注入する。✅
  - **検証メモ（Claude Sonnet 5 引き継ぎ時）**: 実装済みだったがチェックが未反映だったのみ。ビルド確認済み。

- `[x]` **タスクD: カメラ仕様の変更（デバッグカメラ統合）**
  - **対象ファイル**: `GameScene.cpp`, `GameScene.h`, `BaseScene.h`, `BaseScene.cpp`
  - **実装内容**:
    1. エディットモードでも標準レンダリングは `GetMainCamera()` を使用する。✅
    2. ツールバーに「Debug Camera (Free Camera)」トグルを追加し、ONの時のみ `editorCamera_`（`debugCamera_`）を用いて描画・操作を行うように分岐を修正する。✅
  - **【バグ修正】(Claude Sonnet 5)**: 描画分岐は実装済みだったが、`BaseScene::UpdateEditor()` が `isDebugCamera_` を関知せず、常に `editorCamera_->Update()`（入力操作）を実行していた。そのためトグルOFF時は非表示の `editorCamera_` が入力を奪い、表示中の `GetMainCamera()` は一切操作できない状態だった。`BaseScene` に仮想関数 `IsEditorCameraOperationEnabled()`（デフォルト`true`）を追加し、`GameScene` でオーバーライドして `isDebugCamera_` を返すことで、描画と操作のON/OFF分岐を一致させた。

---
## 【フェーズ3：インスペクタ連動とUndo/Redoの実装】

- `[x]` **タスクE: インスペクタとプレビューオブジェクトの完全連動**
  - **対象ファイル**: `TimelineEditorWindow.cpp`, `EditorUIManager.cpp`, `BaseScene.cpp`, `GameObject.h`
  - **実装内容**:
    1. `TimelineManager` に選択中イベント `ITimelineEvent* selectedEvent_` を持たせ、UIの「ひし形」クリックでセットする。✅
    2. `GameObject` に保存対象外フラグ `bool isTimelinePreview_ = false;` を追加しシリアライズから除外。✅（`SceneSerializer::SerializeToString` で除外確認済み）
    3. `EditorUIManager` のインスペクタで、`selectedEvent_` が存在する場合、そのプロパティ（Time, Prefab, XYZ等）を描画する。✅（`DrawTimelineEventInspector`）
    4. `BaseScene` で、`selectedEvent_` に対応するプレビューオブジェクトを動的に生成し、ギズモの変更をイベント座標 `spawnTransform_` へ同期する。非選択時は即座にDestroyする。✅
  - **検証メモ（Claude Sonnet 5 引き継ぎ時）**: 実装済みだったがチェックが未反映だったのみ。コメント内の文字化け（"仓"→"仮"、"顔に書き戻す"→"値に書き戻す"等）を修正。

- `[x]` **タスクF: Undo / Redo の実装 (Command Pattern)**
  - **対象ファイル**: `Command.h`, `EditorUIManager.cpp`, `EditorUIManager.h`, `TimelineEditorWindow.cpp`
  - **実装内容**:
    1. `ICommand` を継承した `TimelineCommand` を作成し、JSON文字列（変更前・変更後）を保持して復元する軽量なUndoを実装する。✅
    2. **【超重要】** 毎フレームスタックが積まれるのを防ぐため、必ず `ImGui::IsItemDeactivatedAfterEdit()` やギズモの操作完了時にのみ Command を発行する。✅（Add/Remove Trackは実装済み。※下記バグ修正参照）
    3. Undo実行等で `TimelineManager::LoadFromJson()` が走る際は、必ず `selectedEvent_ = nullptr` にリセットし、ダングリングポインタを防ぐ。✅（`LoadFromString`内で実施）
  - **【バグ修正】(Claude Sonnet 5)**: `DrawTimelineEventInspector`（Time / Position / Rotation / Scaleの編集）が `IsItemActivated`/`IsItemDeactivatedAfterEdit` を一切見ておらず、Undo/Redoコマンドが全く発行されていなかった（Track追加削除のみ対応済みだった）。加えて `SetSceneModified()` も呼ばれておらず、オートセーブにも乗らなかった。`EditorUIManager` に `timelineSnapshotBeforeEdit_` を追加し、編集開始時にスナップショットを取得、編集完了時に `TimelineCommand` を発行 + `SetSceneModified()` を呼ぶよう修正。

---
## 【引き継ぎ時に発見・修正したビルドエラー】(Claude Sonnet 5)

- `[x]` `TimelineManager::SaveToFile()`（constメンバ関数）内で非mutableな `isDirty_` を変更しておりビルド不能だった（C3490）。`isDirty_` を `mutable` に修正してビルド通過を確認。
- 上記含め、`AbsoluteEngine.vcxproj` / `Application.vcxproj` をフルビルド（Debug/x64, `/WX`警告即エラー設定）して0エラー・0警告を確認済み。`Application.exe` の起動スモークテスト（5秒間クラッシュなし）も実施済み。
  - ※ImGuiベースのDirectXデスクトップアプリのため、ブラウザ経由でのGUI目視確認は未実施。実際のタイムラインエディタ操作感は手動確認を推奨。
