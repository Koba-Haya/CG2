#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Transform.h"
#include "Component.h"
#include "../graphics/3d/model/ModelInstance.h"

namespace AbsoluteEngine {

// Removed hardcoded structs (ColliderInfo, LightComponent, DissolveInfo)
// These are now handled by respective component classes.

class GameObject : public std::enable_shared_from_this<GameObject> {
public:
  GameObject(const std::string& name = "GameObject");
  virtual ~GameObject() = default;

  // 基本プロパティ
  void SetName(const std::string& name) { name_ = name; }
  const std::string& GetName() const { return name_; }

  Transform& GetTransform() { return transform_; }
  const Transform& GetTransform() const { return transform_; }

  // 親子関係
  void AddChild(std::shared_ptr<GameObject> child);
  void RemoveChild(std::shared_ptr<GameObject> child);
  
  std::shared_ptr<GameObject> GetParent() const { return parent_.lock(); }
  const std::vector<std::shared_ptr<GameObject>>& GetChildren() const { return children_; }

  // 毎フレームの更新（コンポーネントと子ノードの更新）
  virtual void Update(float deltaTime);

  // コンポーネント管理
  void AddComponent(std::unique_ptr<IComponent> component);
  void RemoveComponent(IComponent* component);
  const std::vector<std::unique_ptr<IComponent>>& GetComponents() const { return components_; }

  template <typename T>
  T* GetComponent() const {
    for (auto& comp : components_) {
      if (T* t = dynamic_cast<T*>(comp.get())) {
        return t;
      }
    }
    return nullptr;
  }

  template <typename T>
  bool HasComponent() const {
    return GetComponent<T>() != nullptr;
  }

  // オブジェクトと子ノードの描画
  virtual void Draw();

  // メタデータプロパティ
  const std::string& GetTag() const { return tag_; }
  void SetTag(const std::string& tag) { tag_ = tag; }

  // プレハブパス
  const std::string& GetPrefabPath() const { return prefabPath_; }
  void SetPrefabPath(const std::string& path) { prefabPath_ = path; }
  bool IsPrefabInstance() const { return !prefabPath_.empty(); }

  // 生存フラグ
  bool IsActive() const { return isActive_; }
  void Destroy() { isActive_ = false; }

private:
  std::string name_;
  std::string tag_ = "Untagged";
  std::string prefabPath_ = "";

  bool isActive_ = true;

  Transform transform_;

  std::weak_ptr<GameObject> parent_;
  std::vector<std::shared_ptr<GameObject>> children_;
  std::vector<std::unique_ptr<IComponent>> components_;
};

} // namespace AbsoluteEngine
