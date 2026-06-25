#pragma once
#include <vector>
#include <memory>
#include <string>
#include "../Type/Matrix.h"
#include "../Type/Vector.h"
#include "../Type/Transform.h"
#include "CommandManager.h"
#include "../scene/LightNodeComponent.h"

#ifdef USE_IMGUI
#include <imgui.h>
#include "../../externals/ImGuizmo/ImGuizmo.h"
#endif

namespace AbsoluteEngine {

class GameObject;
class EditorCamera;

class EditorUIManager {
public:
  EditorUIManager();
  ~EditorUIManager() = default;

  // 毎フレームのUI描画
  void DrawUI(std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, EditorCamera* camera = nullptr);

  // Viewportへのドラッグ＆ドロップ受付（画像描画後に手動で呼ぶ）
  void HandleViewportDragDrop(std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

  // 現在選択されているオブジェクトを取得
  std::shared_ptr<GameObject> GetSelectedObject() const { return selectedObject_.lock(); }
  void SetSelectedObject(std::shared_ptr<GameObject> obj) { selectedObject_ = obj; }

  // シーンが変更されたかどうかを取得・クリアする
  bool ConsumeSceneModifiedFlag() {
      bool flag = isSceneModified_;
      isSceneModified_ = false;
      return flag;
  }
  void SetSceneModified() { isSceneModified_ = true; }

private:
#ifdef USE_IMGUI
  void DrawMenuBar(std::vector<std::shared_ptr<GameObject>>& rootObjects);
  void DrawToolbar();
  void DrawAssetBrowser(std::vector<std::shared_ptr<GameObject>>& rootObjects);
  void DrawPrefabsBrowser(std::vector<std::shared_ptr<GameObject>>& rootObjects);
  void DrawHierarchy(std::vector<std::shared_ptr<GameObject>>& rootObjects);
  void DrawGameObjectNode(std::shared_ptr<GameObject> obj, std::vector<std::shared_ptr<GameObject>>& rootObjects);
  void DrawInspector();
  void DrawGizmo(std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
  void HandleShortcuts(std::vector<std::shared_ptr<GameObject>>& rootObjects, EditorCamera* camera);
  
  void HandleMousePicking(const std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
  void CheckIntersection(std::shared_ptr<GameObject> obj, const Vector3& rayOrigin, const Vector3& rayDir, std::shared_ptr<GameObject>& hitObject, float& minT);
  void DrawColliderDebug(const std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const ImVec2& vMin, const ImVec2& vMax);
#endif

  std::weak_ptr<GameObject> selectedObject_;
  std::shared_ptr<GameObject> clipboardObject_; // コピー＆ペースト用のバッファ
  std::unique_ptr<CommandManager> commandManager_ = std::make_unique<CommandManager>();

#ifdef USE_IMGUI
  ImGuizmo::OPERATION currentGizmoOperation_ = ImGuizmo::TRANSLATE;
  bool useSnap_ = false;
  float snapValue_ = 1.0f;
  float dropDistance_ = 20.0f; // ドラッグ＆ドロップ時のカメラからの距離
  bool showColliderDebug_ = true;
  
  bool isGizmoUsing_ = false;
  Transform transformBeforeGizmo_;

  Transform transformBeforeInspector_;
  LightNodeComponent lightBeforeInspector_;
#endif
  bool isSceneModified_ = false;
};

} // namespace AbsoluteEngine
