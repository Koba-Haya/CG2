#include "EditorUIManager.h"
#include "../scene/GameObject.h"
#include "../scene/SceneSerializer.h"
#include "../scene/ComponentFactory.h"
#include "EditorCamera.h"
#include "Method.h"
#include <filesystem>

#ifdef USE_IMGUI
#include <imgui.h>
#include "../../externals/ImGuizmo/ImGuizmo.h"
#endif

namespace AbsoluteEngine {

void EditorUIManager::DrawUI(std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, EditorCamera* camera) {
#ifdef USE_IMGUI
  DrawMenuBar(rootObjects);
  DrawToolbar();
  DrawAssetBrowser(rootObjects);
  DrawPrefabsBrowser(rootObjects);
  DrawHierarchy(rootObjects);
  DrawInspector();
  DrawGizmo(rootObjects, viewMatrix, projectionMatrix);
  HandleShortcuts(rootObjects, camera);
#endif
}

#ifdef USE_IMGUI
void EditorUIManager::DrawMenuBar(std::vector<std::shared_ptr<GameObject>>& rootObjects) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      // カレントディレクトリ（実行ファイルの位置）に依存しないよう、プロジェクトフォルダへの絶対パスを指定
      std::string saveDir = "C:/Users/haya2/source/repos/CG2/project/Application/resources/editor/";

      if (ImGui::MenuItem("Save Scene")) {
        std::filesystem::create_directories(saveDir);
        SceneSerializer::Serialize(saveDir + "scene.json", rootObjects);
      }

      if (ImGui::MenuItem("Load Scene")) {
        SceneSerializer::Deserialize(saveDir + "scene.json", rootObjects);
        selectedObject_.reset(); // ロード後は選択をクリア
      }

      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void EditorUIManager::DrawToolbar() {
  ImGui::Begin("Toolbar");
  if (ImGui::RadioButton("Translate (Q)", currentGizmoOperation_ == ImGuizmo::TRANSLATE)) currentGizmoOperation_ = ImGuizmo::TRANSLATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate (W)", currentGizmoOperation_ == ImGuizmo::ROTATE)) currentGizmoOperation_ = ImGuizmo::ROTATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale (R)", currentGizmoOperation_ == ImGuizmo::SCALE)) currentGizmoOperation_ = ImGuizmo::SCALE;
  
  ImGui::Separator();
  ImGui::Checkbox("Snap", &useSnap_);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(100.0f);
  ImGui::DragFloat("Snap Value", &snapValue_, 0.1f, 0.1f, 100.0f);
  
  ImGui::End();
}

void EditorUIManager::DrawAssetBrowser(std::vector<std::shared_ptr<GameObject>>& rootObjects) {
  ImGui::Begin("Assets");
  std::string resourcesPath = "C:/Users/haya2/source/repos/CG2/project/Application/resources";
  
  if (std::filesystem::exists(resourcesPath)) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(resourcesPath)) {
      if (entry.is_regular_file()) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
        std::string payloadType = "";
        
        if (ext == ".obj" || ext == ".gltf") {
          color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f); // Light blue
          payloadType = "ASSET_MODEL_PATH";
        } else if (ext == ".png" || ext == ".jpg" || ext == ".dds") {
          color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green
          payloadType = "ASSET_TEXTURE_PATH";
        } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
          color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // Yellow
        } else if (ext == ".json") {
          color = ImVec4(1.0f, 0.6f, 0.8f, 1.0f); // Pink
          payloadType = "ASSET_PREFAB_PATH";
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        std::string filename = entry.path().filename().string();
        ImGui::Selectable(filename.c_str());
        
        if (!payloadType.empty() && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
          std::string relPath = "resources/" + std::filesystem::relative(entry.path(), resourcesPath).string();
          std::replace(relPath.begin(), relPath.end(), '\\', '/');
          ImGui::SetDragDropPayload(payloadType.c_str(), relPath.c_str(), relPath.size() + 1);
          ImGui::Text("Drag %s", filename.c_str());
          ImGui::EndDragDropSource();
        }
        
        ImGui::PopStyleColor();
      }
    }
  }
  ImGui::End();
}

void EditorUIManager::DrawPrefabsBrowser(std::vector<std::shared_ptr<GameObject>>& rootObjects) {
  ImGui::Begin("Prefabs");
  std::string prefabsPath = "C:/Users/haya2/source/repos/CG2/project/Application/resources/prefabs";
  
  if (std::filesystem::exists(prefabsPath)) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(prefabsPath)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        std::string filename = entry.path().filename().string();
        std::string prefabName = entry.path().stem().string();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.3f, 0.5f, 1.0f));
        
        if (ImGui::Button(prefabName.c_str(), ImVec2(-FLT_MIN, 30))) {
            // クリックでも配置できるようにする
            std::string fullPath = "C:/Users/haya2/source/repos/CG2/project/Application/resources/prefabs/" + filename;
            auto prefabInstance = SceneSerializer::LoadPrefab(fullPath);
            if (prefabInstance) {
                if (commandManager_) {
                  commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(prefabInstance, &rootObjects));
              } else {
                  rootObjects.push_back(prefabInstance);
              }
                selectedObject_ = prefabInstance;
            }
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
          std::string relPath = "resources/prefabs/" + filename;
          ImGui::SetDragDropPayload("ASSET_PREFAB_PATH", relPath.c_str(), relPath.size() + 1);
          ImGui::Text("Place Prefab: %s", prefabName.c_str());
          ImGui::EndDragDropSource();
        }

        ImGui::PopStyleColor(3);
        ImGui::Spacing();
      }
    }
  } else {
    ImGui::Text("No prefabs found.");
  }
  ImGui::End();
}

void EditorUIManager::HandleShortcuts(std::vector<std::shared_ptr<GameObject>>& rootObjects, EditorCamera* camera) {
  // 右クリック押下中はカメラ操作のためショートカット無効
  if (ImGui::IsMouseDown(1)) return;

  ImGuiIO& io = ImGui::GetIO();
  if (!io.WantTextInput) { // テキスト入力中でない場合のみショートカットを有効化
    bool ctrl = io.KeyCtrl;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
        if (commandManager_) commandManager_->Undo();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
        if (commandManager_) commandManager_->Redo();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Q)) currentGizmoOperation_ = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_W)) currentGizmoOperation_ = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) currentGizmoOperation_ = ImGuizmo::SCALE;

    auto sel = selectedObject_.lock();
    if (sel) {
      if (ImGui::IsKeyPressed(ImGuiKey_F) && camera) {
        camera->FocusOn(sel->GetTransform().translate);
      }

      // Deleteキーで削除
      if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        auto parent = sel->GetParent();
        std::shared_ptr<ICommand> cmd;
        if (parent) {
          cmd = std::make_shared<DeleteObjectCommand>(sel, parent);
        } else {
          cmd = std::make_shared<DeleteObjectCommand>(sel, &rootObjects);
        }
        if (commandManager_) commandManager_->ExecuteCommand(cmd);
        selectedObject_.reset();
      }

      // Ctrl+Cでコピー
      if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
        clipboardObject_ = SceneSerializer::CopyGameObject(sel);
      }
    }

    // Ctrl+Vでペースト
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V) && clipboardObject_) {
      auto newObj = SceneSerializer::CopyGameObject(clipboardObject_);
      std::shared_ptr<ICommand> cmd;
      if (sel) {
        cmd = std::make_shared<CreateObjectCommand>(newObj, sel);
      } else {
        cmd = std::make_shared<CreateObjectCommand>(newObj, &rootObjects);
      }
      if (commandManager_) commandManager_->ExecuteCommand(cmd);
      selectedObject_ = newObj; // ペーストしたものを選択状態に
    }
  }
}

void EditorUIManager::DrawGizmo(std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
  // ImGuizmoの設定
  ImGuizmo::SetOrthographic(false);
  ImGuizmo::BeginFrame();

  // "Viewport##GameView" ウィンドウ
  ImGui::Begin("Viewport##GameView");

  // Viewportウィンドウ内でのマウスピッキング処理（BeginとEndの間で行うことでHoveredが正しくとれる）
  HandleMousePicking(rootObjects, viewMatrix, projectionMatrix);

  // タイトルバー等を除いた実際の描画領域を取得する
  ImVec2 vMin = ImGui::GetWindowContentRegionMin();
  ImVec2 vMax = ImGui::GetWindowContentRegionMax();
  ImVec2 wPos = ImGui::GetWindowPos();
  vMin.x += wPos.x;
  vMin.y += wPos.y;
  vMax.x += wPos.x;
  vMax.y += wPos.y;

  // デバッグ描画
  DrawColliderDebug(rootObjects, viewMatrix, projectionMatrix, vMin, vMax);

  auto obj = selectedObject_.lock();
  if (!obj) {
    ImGui::End();
    return;
  }
  ImVec2 windowPos = ImGui::GetWindowPos();
  ImVec2 windowSize = ImGui::GetWindowSize();
  ImGuizmo::SetDrawlist();
  ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

  Transform& t = obj->GetTransform();

  // ImGuizmo用の変換（Translate, Rotate(度数法), Scale）
  float translation[3] = { t.translate.x, t.translate.y, t.translate.z };
  float rotation[3] = { t.rotate.x * 180.0f / 3.14159265f, t.rotate.y * 180.0f / 3.14159265f, t.rotate.z * 180.0f / 3.14159265f };
  float scale[3] = { t.scale.x, t.scale.y, t.scale.z };
  
  float objectMatrix[16];
  ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, objectMatrix);

  float snapValues[3] = { snapValue_, snapValue_, snapValue_ };

  // マニピュレーターの描画と操作
  ImGuizmo::Manipulate(&viewMatrix.m[0][0], &projectionMatrix.m[0][0], currentGizmoOperation_, ImGuizmo::LOCAL, objectMatrix, nullptr, useSnap_ ? snapValues : nullptr);

  // 操作されたら Transform に反映
  bool wasGizmoUsing = isGizmoUsing_;
  isGizmoUsing_ = ImGuizmo::IsUsing();

  if (!wasGizmoUsing && isGizmoUsing_) {
      // 操作開始時
      transformBeforeGizmo_ = obj->GetTransform();
  }

  if (isGizmoUsing_) {
    ImGuizmo::DecomposeMatrixToComponents(objectMatrix, translation, rotation, scale);
    t.translate = { translation[0], translation[1], translation[2] };
    t.rotate = { rotation[0] * 3.14159265f / 180.0f, rotation[1] * 3.14159265f / 180.0f, rotation[2] * 3.14159265f / 180.0f };
    t.scale = { scale[0], scale[1], scale[2] };
  }

  if (wasGizmoUsing && !isGizmoUsing_) {
      // 操作終了時
      auto afterTransform = obj->GetTransform();
      if (transformBeforeGizmo_.translate.x != afterTransform.translate.x || transformBeforeGizmo_.translate.y != afterTransform.translate.y || transformBeforeGizmo_.translate.z != afterTransform.translate.z ||
          transformBeforeGizmo_.rotate.x != afterTransform.rotate.x || transformBeforeGizmo_.rotate.y != afterTransform.rotate.y || transformBeforeGizmo_.rotate.z != afterTransform.rotate.z ||
          transformBeforeGizmo_.scale.x != afterTransform.scale.x || transformBeforeGizmo_.scale.y != afterTransform.scale.y || transformBeforeGizmo_.scale.z != afterTransform.scale.z) {
          
          auto cmd = std::make_shared<TransformCommand>(obj, transformBeforeGizmo_, afterTransform);
          if (commandManager_) {
              commandManager_->AddCommand(cmd);
          }
      }
  }

  ImGui::End();
}

void EditorUIManager::HandleMousePicking(const std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
  // Viewportウィンドウがホバーされているか、ギズモを操作中でないか確認
  if (!ImGui::IsWindowHovered() || ImGuizmo::IsOver()) return;

  // 左クリックされた瞬間のみ判定
  if (ImGui::IsMouseClicked(0)) {
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // タイトルバー等を除いた実際の描画領域を取得する
    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
    ImVec2 vMax = ImGui::GetWindowContentRegionMax();
    ImVec2 wPos = ImGui::GetWindowPos();
    vMin.x += wPos.x;
    vMin.y += wPos.y;
    vMax.x += wPos.x;
    vMax.y += wPos.y;

    // 描画領域内での割合 (0.0 ~ 1.0)
    float u = (mousePos.x - vMin.x) / (vMax.x - vMin.x);
    float v = (mousePos.y - vMin.y) / (vMax.y - vMin.y);

    // 領域外をクリックした場合は何もしない（一応チェック）
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return;

    // NDC（正規化デバイス座標系: -1.0 ~ 1.0, Y軸反転）
    float ndcX = u * 2.0f - 1.0f;
    float ndcY = 1.0f - v * 2.0f;

    // ViewProjectionの逆行列を計算
    Matrix4x4 viewProj = Multiply(viewMatrix, projectionMatrix);
    Matrix4x4 invViewProj = Inverse(viewProj);

    // Nearクリップ面とFarクリップ面でのワールド座標を計算
    Vector3 nearPos = TransformPoint({ ndcX, ndcY, 0.0f }, invViewProj);
    Vector3 farPos = TransformPoint({ ndcX, ndcY, 1.0f }, invViewProj);

    // レイの方向ベクトル
    Vector3 rayDir = Normalize({ farPos.x - nearPos.x, farPos.y - nearPos.y, farPos.z - nearPos.z });

    // 一番手前で交差したオブジェクトを探す
    std::shared_ptr<GameObject> hitObject = nullptr;
    float minT = FLT_MAX; // 距離の最小値

    for (const auto& obj : rootObjects) {
      CheckIntersection(obj, nearPos, rayDir, hitObject, minT);
    }

    // 選択状態を更新
    if (hitObject) {
      selectedObject_ = hitObject;
    } else {
      selectedObject_.reset();
    }
  }
}

void EditorUIManager::CheckIntersection(std::shared_ptr<GameObject> obj, const Vector3& rayOrigin, const Vector3& rayDir, std::shared_ptr<GameObject>& hitObject, float& minT) {
  if (!obj) return;

  const ColliderInfo& collider = obj->GetCollider();
  if (collider.type != ColliderInfo::Type::None) {
    Vector3 objPos = obj->GetTransform().translate;
    Vector3 scale = obj->GetTransform().scale;
    Vector3 center = { objPos.x + collider.centerOffset.x * scale.x, 
                       objPos.y + collider.centerOffset.y * scale.y, 
                       objPos.z + collider.centerOffset.z * scale.z };

    if (collider.type == ColliderInfo::Type::Sphere) {
      float maxScale = (std::max)({ scale.x, scale.y, scale.z });
      float scaledRadius = collider.radius * maxScale;

      Vector3 m = { rayOrigin.x - center.x, rayOrigin.y - center.y, rayOrigin.z - center.z };
      float b = Dot(m, rayDir);
      float c = Dot(m, m) - scaledRadius * scaledRadius;

      float discriminant = b * b - c;
      if (discriminant > 0.0f) {
        float t = -b - std::sqrt(discriminant);
        if (t > 0.0f && t < minT) {
          minT = t;
          hitObject = obj;
        }
      }
    } else if (collider.type == ColliderInfo::Type::AABB) {
      Vector3 scaledSize = { collider.size.x * scale.x, collider.size.y * scale.y, collider.size.z * scale.z };
      Vector3 minBounds = { center.x - scaledSize.x, center.y - scaledSize.y, center.z - scaledSize.z };
      Vector3 maxBounds = { center.x + scaledSize.x, center.y + scaledSize.y, center.z + scaledSize.z };
      
      float t1 = (minBounds.x - rayOrigin.x) / (rayDir.x != 0.0f ? rayDir.x : 0.00001f);
      float t2 = (maxBounds.x - rayOrigin.x) / (rayDir.x != 0.0f ? rayDir.x : 0.00001f);
      float t3 = (minBounds.y - rayOrigin.y) / (rayDir.y != 0.0f ? rayDir.y : 0.00001f);
      float t4 = (maxBounds.y - rayOrigin.y) / (rayDir.y != 0.0f ? rayDir.y : 0.00001f);
      float t5 = (minBounds.z - rayOrigin.z) / (rayDir.z != 0.0f ? rayDir.z : 0.00001f);
      float t6 = (maxBounds.z - rayOrigin.z) / (rayDir.z != 0.0f ? rayDir.z : 0.00001f);
      
      float tmin = (std::max)((std::max)((std::min)(t1, t2), (std::min)(t3, t4)), (std::min)(t5, t6));
      float tmax = (std::min)((std::min)((std::max)(t1, t2), (std::max)(t3, t4)), (std::max)(t5, t6));
      
      if (tmax >= 0 && tmin <= tmax) {
        if (tmin < minT) {
          minT = tmin;
          hitObject = obj;
        }
      }
    }
  }

  // 子ノードも再帰的にチェック
  for (const auto& child : obj->GetChildren()) {
    CheckIntersection(child, rayOrigin, rayDir, hitObject, minT);
  }
}

static bool WorldToScreen(const Vector3& worldPos, const Matrix4x4& viewProj, const ImVec2& vMin, const ImVec2& vMax, ImVec2& outScreenPos) {
  Vector3 clipCoords = TransformPoint(worldPos, viewProj);
  if (clipCoords.z < 0.0f || clipCoords.z > 1.0f) return false;

  float u = (clipCoords.x + 1.0f) * 0.5f;
  float v = (1.0f - clipCoords.y) * 0.5f;

  outScreenPos.x = vMin.x + u * (vMax.x - vMin.x);
  outScreenPos.y = vMin.y + v * (vMax.y - vMin.y);
  return true;
}

void EditorUIManager::DrawColliderDebug(const std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const ImVec2& vMin, const ImVec2& vMax) {
  Matrix4x4 viewProj = Multiply(viewMatrix, projectionMatrix);
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  ImU32 color = IM_COL32(0, 255, 0, 255); // 緑色

  auto drawObj = [&](auto& self, const std::shared_ptr<GameObject>& obj) -> void {
    if (!obj) return;

    const ColliderInfo& collider = obj->GetCollider();
    if (collider.type != ColliderInfo::Type::None) {
      Vector3 objPos = obj->GetTransform().translate;
      Vector3 scale = obj->GetTransform().scale;
      Vector3 center = { objPos.x + collider.centerOffset.x * scale.x, 
                         objPos.y + collider.centerOffset.y * scale.y, 
                         objPos.z + collider.centerOffset.z * scale.z };

      if (collider.type == ColliderInfo::Type::AABB) {
        Vector3 scaledSize = { collider.size.x * scale.x, collider.size.y * scale.y, collider.size.z * scale.z };
        
        Vector3 corners[8] = {
          {center.x - scaledSize.x, center.y - scaledSize.y, center.z - scaledSize.z},
          {center.x + scaledSize.x, center.y - scaledSize.y, center.z - scaledSize.z},
          {center.x + scaledSize.x, center.y + scaledSize.y, center.z - scaledSize.z},
          {center.x - scaledSize.x, center.y + scaledSize.y, center.z - scaledSize.z},
          {center.x - scaledSize.x, center.y - scaledSize.y, center.z + scaledSize.z},
          {center.x + scaledSize.x, center.y - scaledSize.y, center.z + scaledSize.z},
          {center.x + scaledSize.x, center.y + scaledSize.y, center.z + scaledSize.z},
          {center.x - scaledSize.x, center.y + scaledSize.y, center.z + scaledSize.z}
        };

        ImVec2 screenCorners[8];
        bool visible[8];
        for (int i = 0; i < 8; ++i) {
          visible[i] = WorldToScreen(corners[i], viewProj, vMin, vMax, screenCorners[i]);
        }

        auto drawLine = [&](int i, int j) {
          if (visible[i] && visible[j]) {
            drawList->AddLine(screenCorners[i], screenCorners[j], color);
          }
        };

        drawLine(0, 1); drawLine(1, 2); drawLine(2, 3); drawLine(3, 0);
        drawLine(4, 5); drawLine(5, 6); drawLine(6, 7); drawLine(7, 4);
        drawLine(0, 4); drawLine(1, 5); drawLine(2, 6); drawLine(3, 7);

      } else if (collider.type == ColliderInfo::Type::Sphere) {
        float maxScale = (std::max)({ scale.x, scale.y, scale.z });
        float r = collider.radius * maxScale;
        
        Vector3 points[6] = {
          {center.x - r, center.y, center.z}, {center.x + r, center.y, center.z},
          {center.x, center.y - r, center.z}, {center.x, center.y + r, center.z},
          {center.x, center.y, center.z - r}, {center.x, center.y, center.z + r}
        };
        ImVec2 sp[6];
        bool v[6];
        for (int i = 0; i < 6; ++i) v[i] = WorldToScreen(points[i], viewProj, vMin, vMax, sp[i]);

        if(v[0]&&v[1]) drawList->AddLine(sp[0], sp[1], color);
        if(v[2]&&v[3]) drawList->AddLine(sp[2], sp[3], color);
        if(v[4]&&v[5]) drawList->AddLine(sp[4], sp[5], color);
      }
    }

    for (const auto& child : obj->GetChildren()) {
      self(self, child);
    }
  };

  for (const auto& root : rootObjects) {
    drawObj(drawObj, root);
  }
}
#endif
#ifdef USE_IMGUI
void EditorUIManager::DrawHierarchy(std::vector<std::shared_ptr<GameObject>>& rootObjects) {
  ImGui::Begin("Hierarchy");

  // 階層のルートオブジェクトからツリーを描画
  for (const auto& obj : rootObjects) {
    DrawGameObjectNode(obj, rootObjects);
  }

  // Hierarchyウィンドウ全体へのドロップ受け入れ
  ImGui::Dummy(ImGui::GetContentRegionAvail()); // 残りのスペースをダミーで埋める
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PREFAB_PATH")) {
      const char* payloadPath = (const char*)payload->Data;
      std::string fullPath = "C:/Users/haya2/source/repos/CG2/project/Application/" + std::string(payloadPath);
      auto prefabInstance = SceneSerializer::LoadPrefab(fullPath);
      if (prefabInstance) {
        if (commandManager_) {
          commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(prefabInstance, &rootObjects));
        } else {
          rootObjects.push_back(prefabInstance);
        }
        selectedObject_ = prefabInstance;
      }
    }
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MODEL_PATH")) {
      const char* payloadPath = (const char*)payload->Data;
      std::filesystem::path p(payloadPath);
      std::string objName = p.stem().string();
      if (objName.empty()) objName = "Model";

      auto newObj = std::make_shared<AbsoluteEngine::GameObject>(objName);
      newObj->LoadModel(payloadPath);
      newObj->GetTransform().translate = {0, 0, 0};
      
      if (commandManager_) {
        commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(newObj, &rootObjects));
      } else {
        rootObjects.push_back(newObj);
      }
      selectedObject_ = newObj;
    }
    ImGui::EndDragDropTarget();
  }

  // 余白をクリックしたら選択解除
  if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
    selectedObject_.reset();
  }

  // 右クリックメニュー（新規作成）
  if (ImGui::BeginPopupContextWindow("HierarchyContextWindow", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
      if (ImGui::Selectable("Create Empty")) {
          auto newObj = std::make_shared<GameObject>("GameObject");
          if (commandManager_) {
              commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(newObj, &rootObjects));
          } else {
              rootObjects.push_back(newObj);
          }
          selectedObject_ = newObj;
      }
      if (ImGui::BeginMenu("Create Light")) {
          if (ImGui::Selectable("Directional Light")) {
              auto newObj = std::make_shared<GameObject>("Directional Light");
              newObj->GetLight().type = LightComponent::Type::Directional;
              newObj->GetTransform().rotate = { 0.5f, 0.5f, 0.0f };
              if (commandManager_) { commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(newObj, &rootObjects)); } else { rootObjects.push_back(newObj); }
              selectedObject_ = newObj;
          }
          if (ImGui::Selectable("Point Light")) {
              auto newObj = std::make_shared<GameObject>("Point Light");
              newObj->GetLight().type = LightComponent::Type::Point;
              if (commandManager_) { commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(newObj, &rootObjects)); } else { rootObjects.push_back(newObj); }
              selectedObject_ = newObj;
          }
          if (ImGui::Selectable("Spot Light")) {
              auto newObj = std::make_shared<GameObject>("Spot Light");
              newObj->GetLight().type = LightComponent::Type::Spot;
              if (commandManager_) { commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(newObj, &rootObjects)); } else { rootObjects.push_back(newObj); }
              selectedObject_ = newObj;
          }
          ImGui::EndMenu();
      }
      ImGui::EndPopup();
  }

  ImGui::End();
}

void EditorUIManager::DrawGameObjectNode(std::shared_ptr<GameObject> obj, std::vector<std::shared_ptr<GameObject>>& rootObjects) {
  if (!obj) return;

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
  
  // 現在選択されているノードならハイライト
  if (selectedObject_.lock() == obj) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  
  // 子がいない場合は葉ノードとして表示
  if (obj->GetChildren().empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  }

  bool isOpen = ImGui::TreeNodeEx((void*)obj.get(), flags, "%s", obj->GetName().c_str());

  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    selectedObject_ = obj;
  }

  if (isOpen && !obj->GetChildren().empty()) {
    for (const auto& child : obj->GetChildren()) {
      DrawGameObjectNode(child, rootObjects);
    }
    ImGui::TreePop();
  }
}

void EditorUIManager::DrawInspector() {
  ImGui::Begin("Inspector");

  auto obj = selectedObject_.lock();
  if (obj) {
    // 名前の編集
    char nameBuffer[256];
    strncpy_s(nameBuffer, obj->GetName().c_str(), sizeof(nameBuffer));
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
      obj->SetName(nameBuffer);
    }

    // タグの編集
    char tagBuffer[256];
    strncpy_s(tagBuffer, obj->GetTag().c_str(), sizeof(tagBuffer));
    if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer))) {
      obj->SetTag(tagBuffer);
    }

    ImGui::Separator();
    
    // モデルとテクスチャの設定表示
    if (ImGui::CollapsingHeader("Model & Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Model: %s", obj->GetModelPath().empty() ? "None" : obj->GetModelPath().c_str());
      ImGui::Text("Texture: %s", obj->GetTexturePath().empty() ? "None" : obj->GetTexturePath().c_str());
      
      // テクスチャ適用用のドロップエリア
      ImGui::Button("Drop Texture Here (.png/.dds)", ImVec2(-FLT_MIN, 30));
      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE_PATH")) {
          const char* payloadPath = (const char*)payload->Data;
          obj->LoadTexture(payloadPath);
        }
        ImGui::EndDragDropTarget();
      }
    }

    ImGui::Separator();

    // Transformの編集
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
      Transform& transform = obj->GetTransform();

      // Vector3の配列としてImGuiに渡すためのテンポラリ変数
      float t[3] = { transform.translate.x, transform.translate.y, transform.translate.z };
      float r[3] = { transform.rotate.x, transform.rotate.y, transform.rotate.z };
      float s[3] = { transform.scale.x, transform.scale.y, transform.scale.z };

      bool activated = false;
      bool deactivatedAfterEdit = false;

      if (ImGui::DragFloat3("Position", t, 0.1f)) {
        transform.translate = { t[0], t[1], t[2] };
      }
      activated |= ImGui::IsItemActivated();
      deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();

      if (ImGui::DragFloat3("Rotation", r, 0.01f)) {
        transform.rotate = { r[0], r[1], r[2] };
      }
      activated |= ImGui::IsItemActivated();
      deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();

      if (ImGui::DragFloat3("Scale", s, 0.1f)) {
        transform.scale = { s[0], s[1], s[2] };
      }
      activated |= ImGui::IsItemActivated();
      deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();

      if (activated) {
          // 入力開始時のTransformを保存（DragFloat開始前）
          // t, r, s の値は画面上の値なので、実際のtransformの値を保持する
          // ※DragFloatの同じフレームで呼ばれるとすでに変わっている可能性があるので注意
          transformBeforeInspector_.translate = { t[0], t[1], t[2] };
          transformBeforeInspector_.rotate = { r[0], r[1], r[2] };
          transformBeforeInspector_.scale = { s[0], s[1], s[2] };
      }

      if (deactivatedAfterEdit) {
          if (commandManager_) {
              commandManager_->AddCommand(std::make_shared<TransformCommand>(obj, transformBeforeInspector_, transform));
          }
      }
    }

    // --- LightComponent ---
    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
      LightComponent& l = obj->GetLight();
      int currentType = static_cast<int>(l.type);
      const char* lightTypes[] = { "None", "Directional", "Point", "Spot" };

      bool activated = false;
      bool deactivatedAfterEdit = false;

      if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes))) {
        l.type = static_cast<LightComponent::Type>(currentType);
      }
      activated |= ImGui::IsItemActivated();
      deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();

      if (l.type != LightComponent::Type::None) {
        float color[3] = { l.color.x, l.color.y, l.color.z };
        if (ImGui::ColorEdit3("Color", color)) {
          l.color = { color[0], color[1], color[2] };
        }
        activated |= ImGui::IsItemActivated();
        deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();

        ImGui::DragFloat("Intensity", &l.intensity, 0.05f, 0.0f, 100.0f);
        activated |= ImGui::IsItemActivated();
        deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();
        
        if (l.type == LightComponent::Type::Point || l.type == LightComponent::Type::Spot) {
          ImGui::DragFloat("Radius", &l.radius, 0.1f, 0.0f, 1000.0f);
          activated |= ImGui::IsItemActivated();
          deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();

          ImGui::DragFloat("Decay", &l.decay, 0.05f, 0.0f, 10.0f);
          activated |= ImGui::IsItemActivated();
          deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();
        }
        if (l.type == LightComponent::Type::Spot) {
          ImGui::DragFloat("Distance", &l.distance, 0.1f, 0.0f, 1000.0f);
          activated |= ImGui::IsItemActivated();
          deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();

          ImGui::DragFloat("ConeAngle (deg)", &l.coneAngleDeg, 0.5f, 0.0f, 180.0f);
          activated |= ImGui::IsItemActivated();
          deactivatedAfterEdit |= ImGui::IsItemDeactivatedAfterEdit();
        }
      }

      if (activated) {
          lightBeforeInspector_ = obj->GetLight();
      }
      if (deactivatedAfterEdit) {
          if (commandManager_) {
              commandManager_->AddCommand(std::make_shared<LightCommand>(obj, lightBeforeInspector_, l));
          }
      }
    }

    // --- Dissolve ---
    if (ImGui::CollapsingHeader("Dissolve", ImGuiTreeNodeFlags_DefaultOpen)) {
      auto& dissolve = obj->GetDissolve();
      ImGui::Checkbox("Enable Dissolve", &dissolve.enable);
      if (dissolve.enable) {
        ImGui::SliderFloat("Threshold", &dissolve.threshold, 0.0f, 1.0f);
        ImGui::SliderFloat("Edge Range", &dissolve.edgeRange, 0.0f, 0.1f);
        float edgeCol[3] = { dissolve.edgeColor.x, dissolve.edgeColor.y, dissolve.edgeColor.z };
        if (ImGui::ColorEdit3("Edge Color", edgeCol)) {
          dissolve.edgeColor = { edgeCol[0], edgeCol[1], edgeCol[2] };
        }
        float maskCol[3] = { dissolve.maskColor.x, dissolve.maskColor.y, dissolve.maskColor.z };
        if (ImGui::ColorEdit3("Mask Color", maskCol)) {
          dissolve.maskColor = { maskCol[0], maskCol[1], maskCol[2] };
        }
      }
    }

    // Colliderの編集
    if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
      ColliderInfo& collider = obj->GetCollider();

      const char* types[] = { "None", "Sphere", "AABB" };
      int currentType = static_cast<int>(collider.type);
      if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types))) {
        collider.type = static_cast<ColliderInfo::Type>(currentType);
      }

      if (collider.type != ColliderInfo::Type::None) {
        ImGui::DragFloat3("Center Offset", &collider.centerOffset.x, 0.1f);

        if (collider.type == ColliderInfo::Type::Sphere) {
          ImGui::DragFloat("Radius", &collider.radius, 0.1f, 0.0f);
        } else if (collider.type == ColliderInfo::Type::AABB) {
          ImGui::DragFloat3("Size (Half Extents)", &collider.size.x, 0.1f, 0.0f);
        }
      }
    }

    ImGui::Separator();
    
    // プレハブ関連操作
    if (ImGui::CollapsingHeader("Prefab", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (obj->IsPrefabInstance()) {
        ImGui::Text("Prefab: %s", obj->GetPrefabPath().c_str());
      } else {
        ImGui::Text("Prefab: Not a prefab instance.");
      }

      if (ImGui::Button("Save as Prefab", ImVec2(-FLT_MIN, 30))) {
        std::string prefabDir = "C:/Users/haya2/source/repos/CG2/project/Application/resources/prefabs/";
        std::filesystem::create_directories(prefabDir);
        std::string filename = obj->GetName() + ".json";
        std::string filepath = prefabDir + filename;
        if (SceneSerializer::SavePrefab(filepath, obj)) {
            obj->SetPrefabPath(filepath);
        }
      }
    }

    ImGui::Separator();

    // コンポーネント管理
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen)) {
      IComponent* componentToRemove = nullptr;
      
      for (const auto& comp : obj->GetComponents()) {
        ImGui::Text("- %s", comp->GetTypeName().c_str());
        ImGui::SameLine();
        
        // ボタンIDを一意にするためにポインタアドレスを使用
        std::string btnLabel = "Remove##" + std::to_string(reinterpret_cast<uintptr_t>(comp.get()));
        if (ImGui::Button(btnLabel.c_str())) {
            componentToRemove = comp.get();
        }
        // 今後はここで comp->DrawInspector() などを呼んでパラメータ編集できるようにする
      }

      if (componentToRemove) {
          obj->RemoveComponent(componentToRemove);
      }

      ImGui::Spacing();
      if (ImGui::Button("Add Component", ImVec2(-FLT_MIN, 30))) {
        ImGui::OpenPopup("AddComponentPopup");
      }

      if (ImGui::BeginPopup("AddComponentPopup")) {
        auto names = ComponentFactory::GetInstance().GetRegisteredComponentNames();
        if (names.empty()) {
          ImGui::TextDisabled("No components registered.");
        } else {
          for (const auto& name : names) {
            if (ImGui::Selectable(name.c_str())) {
              auto comp = ComponentFactory::GetInstance().Create(name);
              if (comp) {
                obj->AddComponent(std::move(comp));
              }
            }
          }
        }
        ImGui::EndPopup();
      }
    }

  } else {
    ImGui::Text("No Object Selected");
  }

  ImGui::End();
}
#endif

void EditorUIManager::HandleViewportDragDrop(std::vector<std::shared_ptr<GameObject>>& rootObjects, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
#ifdef USE_IMGUI
  if (ImGui::BeginDragDropTarget()) {
    
    // 配置先のワールド座標を計算
    Vector3 targetPos = {0, 0, 0};
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
    ImVec2 vMax = ImGui::GetWindowContentRegionMax();
    ImVec2 wPos = ImGui::GetWindowPos();
    vMin.x += wPos.x;
    vMin.y += wPos.y;
    vMax.x += wPos.x;
    vMax.y += wPos.y;

    float u = (mousePos.x - vMin.x) / (vMax.x - vMin.x);
    float v = (mousePos.y - vMin.y) / (vMax.y - vMin.y);

    if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            dropDistance_ += wheel * 2.0f; // ホイール1段階につき2m移動
            if (dropDistance_ < 1.0f) dropDistance_ = 1.0f; // 最小値
        }

        float ndcX = u * 2.0f - 1.0f;
        float ndcY = 1.0f - v * 2.0f;

        Matrix4x4 viewProj = Multiply(viewMatrix, projectionMatrix);
        Matrix4x4 invViewProj = Inverse(viewProj);

        Vector3 nearPos = TransformPoint({ ndcX, ndcY, 0.0f }, invViewProj);
        Vector3 farPos = TransformPoint({ ndcX, ndcY, 1.0f }, invViewProj);
        Vector3 rayDir = Normalize({ farPos.x - nearPos.x, farPos.y - nearPos.y, farPos.z - nearPos.z });

        // カメラから一定距離(dropDistance_)の座標をドロップ先とする
        targetPos = { nearPos.x + rayDir.x * dropDistance_, nearPos.y + rayDir.y * dropDistance_, nearPos.z + rayDir.z * dropDistance_ };

        // プレビュー表示: マーカーを描画
        ImVec2 screenPos;
        if (WorldToScreen(targetPos, viewProj, vMin, vMax, screenPos)) {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            // 距離を表示するテキストも追加すると親切
            char distText[32];
            snprintf(distText, sizeof(distText), "%.1fm", dropDistance_);
            drawList->AddText(ImVec2(screenPos.x + 10, screenPos.y + 10), IM_COL32(255, 255, 0, 255), distText);
            
            drawList->AddCircle(screenPos, 5.0f, IM_COL32(255, 255, 0, 255), 12, 2.0f);
            drawList->AddLine(ImVec2(screenPos.x - 10, screenPos.y), ImVec2(screenPos.x + 10, screenPos.y), IM_COL32(255, 255, 0, 255), 2.0f);
            drawList->AddLine(ImVec2(screenPos.x, screenPos.y - 10), ImVec2(screenPos.x, screenPos.y + 10), IM_COL32(255, 255, 0, 255), 2.0f);
        }
    }

    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PREFAB_PATH", ImGuiDragDropFlags_AcceptBeforeDelivery)) {
      if (payload->IsDelivery()) {
        const char* payloadPath = (const char*)payload->Data;
        std::string fullPath = "C:/Users/haya2/source/repos/CG2/project/Application/" + std::string(payloadPath);
        auto prefabInstance = SceneSerializer::LoadPrefab(fullPath);
        if (prefabInstance) {
          prefabInstance->GetTransform().translate = targetPos;
          if (commandManager_) {
            commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(prefabInstance, &rootObjects));
          } else {
            rootObjects.push_back(prefabInstance);
          }
          selectedObject_ = prefabInstance;
        }
      }
    }
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MODEL_PATH", ImGuiDragDropFlags_AcceptBeforeDelivery)) {
      if (payload->IsDelivery()) {
        const char* payloadPath = (const char*)payload->Data;
        std::filesystem::path p(payloadPath);
        std::string objName = p.stem().string();
        if (objName.empty()) objName = "Model";

        auto newObj = std::make_shared<AbsoluteEngine::GameObject>(objName);
        newObj->LoadModel(payloadPath);
        newObj->GetTransform().translate = targetPos;
        
        if (commandManager_) {
          commandManager_->ExecuteCommand(std::make_shared<CreateObjectCommand>(newObj, &rootObjects));
        } else {
          rootObjects.push_back(newObj);
        }
        selectedObject_ = newObj;
      }
    }
    ImGui::EndDragDropTarget();
  }
#endif
}

} // namespace AbsoluteEngine
