#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Transform.h"

namespace AbsoluteEngine {

struct ColliderInfo {
  enum class Type {
    None,
    Sphere,
    AABB
  };
  Type type = Type::Sphere; // デフォルトでSphere（第3段階の仕様と互換）
  Vector3 centerOffset = { 0.0f, 0.0f, 0.0f };
  float radius = 1.0f; // Sphere用
  Vector3 size = { 1.0f, 1.0f, 1.0f }; // AABB用
};

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

  // 毎フレームの更新（派生クラスでオーバーライドするか、コンポーネントを回す）
  virtual void Update(float deltaTime);

  // メタデータプロパティ
  const std::string& GetTag() const { return tag_; }
  void SetTag(const std::string& tag) { tag_ = tag; }

  ColliderInfo& GetCollider() { return collider_; }
  const ColliderInfo& GetCollider() const { return collider_; }

private:
  std::string name_;
  std::string tag_ = "Untagged";
  Transform transform_;
  ColliderInfo collider_;

  std::weak_ptr<GameObject> parent_;
  std::vector<std::shared_ptr<GameObject>> children_;
};

} // namespace AbsoluteEngine
