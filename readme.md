# CG2 課題成果まとめ

## 加点要素対応状況

| 項目 | 説明 |
|------|------|
| 球の描画                | `sphere.obj` を描画し、SRT（スケール・回転・移動）を ImGui で操作可能。                               |
| Lambertian Reflectance | Lambert反射による陰影表現。Directional Light の方向・色・強度を変更可能。                              |
| Half Lambert           | Half Lambert を選択可能で、より柔らかい陰影表現に切り替え可能。                                        |
| UVTransform            | テクスチャのUVオフセット、スケール、回転を ImGui から変更できる。                                       |
| 複数モデルの描画         | `sphere.obj` と `plane.obj` を同時に描画。SRTも個別に変更可能。                                      |
| Lighting方式の変更      | `None`, `Lambert`, `Half Lambert` の3種類から動的に切り替え可能。                                    |
| Sound                  | ImGuiで「再生」ボタンを押すと音声が再生される機能を追加。多重再生を防ぐため、再生中の音は自動で停止される。 |
| ドキュメント            | 本READMEに成果と仕組みを整理。加点要素との対応を明記。                                                 |
| その他                  | 球を中心に回転できるデバッグカメラを実装。ImGuiでカメラの移動・回転を個別に操作可能。                    | 

---

## 実装ポイント詳細

### モデル描画と操作
- 複数のモデル（球・平面）をそれぞれ別のバッファ・Transformで管理。
- ImGui 上でモデルごとに Translate / Rotate / Scale をリアルタイムに調整できる。

### ライティング表現
- DirectionalLight の向き・色・強度を変更でき、Lambert／Half Lambert で見え方が変化する。
- Intensity スライダーで明るさを動的に制御できる。

### マテリアルとUVの制御
- テクスチャのUVTransform（平行移動・拡縮・回転）を ImGui から制御できる。
- Sprite と同様に、個別のマテリアル設定も行える。

### Lighting方式の動的変更
- None / Lambert / Half Lambert の3つから動的に切替ができる。

### サウンド再生
- 初期状態で自動再生は行わず、ImGuiの「再生」ボタンを押すことで音声を再生。
- サウンドが重複して鳴らないよう、再生中の音を `Stop()` → `FlushSourceBuffers()` してから `Play()` するように `SoundPlayWave()` を改修。
- これにより任意のタイミングで単独再生が可能に。

### デバッグカメラ（その他）
- 球モデルを中心に、カメラがピボット回転できるデバッグカメラを実装。
- ImGui 上からカメラの移動・X/Y/Z 各軸の回転を個別にリアルタイム操作可能。
- 視点の変化による描画検証がしやすく、デバッグ用途としても有効。