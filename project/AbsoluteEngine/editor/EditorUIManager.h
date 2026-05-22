#pragma once
#include <vector>
#include <memory>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include "../Type/Matrix.h"
#include "../Type/Vector.h"

namespace AbsoluteEngine {

class GameObject;

class EditorUIManager {
public:
  EditorUIManager() = default;
  ~EditorUIManager() = default;

  // 毎フレームのUI描画
  void DrawUI(std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

  // 現在選択されているオブジェクトを取得
  std::shared_ptr<GameObject> GetSelectedObject() const { return selectedObject_.lock(); }

private:
#ifdef USE_IMGUI
  void DrawMenuBar(std::vector<std::shared_ptr<GameObject>>& rootObjects);
  void DrawHierarchy(const std::vector<std::shared_ptr<GameObject>>& rootObjects);
  void DrawGameObjectNode(std::shared_ptr<GameObject> obj);
  void DrawInspector();
  void DrawGizmo(const std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
  
  void HandleMousePicking(const std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
  void CheckIntersection(std::shared_ptr<GameObject> obj, const Vector3& rayOrigin, const Vector3& rayDir, std::shared_ptr<GameObject>& hitObject, float& minT);
  void DrawColliderDebug(const std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const struct ImVec2& vMin, const struct ImVec2& vMax);
#endif

  std::weak_ptr<GameObject> selectedObject_;
};

} // namespace AbsoluteEngine
