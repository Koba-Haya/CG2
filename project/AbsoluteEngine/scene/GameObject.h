#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Transform.h"
#include "Component.h"
#include "../graphics/3d/model/ModelInstance.h"

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

struct LightComponent {
  enum class Type { None, Directional, Point, Spot };
  Type type = Type::None;
  Vector3 color = {1.0f, 1.0f, 1.0f};
  float intensity = 1.0f;
  float radius = 10.0f;       // Point, Spot用
  float decay = 2.0f;         // Point, Spot用
  float distance = 10.0f;     // Spot用
  float coneAngleDeg = 30.0f; // Spot用
};

struct DissolveInfo {
  bool enable = false;
  float threshold = 0.5f;
  float edgeRange = 0.03f;
  Vector3 edgeColor = {1.0f, 0.4f, 0.3f};
  Vector3 maskColor = {1.0f, 1.0f, 1.0f};
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

  // 毎フレームの更新（コンポーネントと子ノードの更新）
  virtual void Update(float deltaTime);

  // コンポーネント管理
  void AddComponent(std::unique_ptr<IComponent> component);
  void RemoveComponent(IComponent* component);
  const std::vector<std::unique_ptr<IComponent>>& GetComponents() const { return components_; }

  // オブジェクトと子ノードの描画
  virtual void Draw();

  // モデルとテクスチャのロード
  void LoadModel(const std::string& path);
  void LoadTexture(const std::string& path);

  const std::string& GetModelPath() const { return modelPath_; }
  const std::string& GetTexturePath() const { return texturePath_; }

  // メタデータプロパティ
  const std::string& GetTag() const { return tag_; }
  void SetTag(const std::string& tag) { tag_ = tag; }

  // プレハブパス
  const std::string& GetPrefabPath() const { return prefabPath_; }
  void SetPrefabPath(const std::string& path) { prefabPath_ = path; }
  bool IsPrefabInstance() const { return !prefabPath_.empty(); }

  ColliderInfo& GetCollider() { return collider_; }
  const ColliderInfo& GetCollider() const { return collider_; }

  LightComponent& GetLight() { return light_; }
  const LightComponent& GetLight() const { return light_; }

  DissolveInfo& GetDissolve() { return dissolve_; }
  const DissolveInfo& GetDissolve() const { return dissolve_; }

private:
  std::string name_;
  std::string tag_ = "Untagged";
  std::string prefabPath_ = "";
  Transform transform_;
  ColliderInfo collider_;
  LightComponent light_;
  DissolveInfo dissolve_;

  std::string modelPath_;
  std::string texturePath_;
  std::unique_ptr<ModelInstance> modelInstance_;

  std::weak_ptr<GameObject> parent_;
  std::vector<std::shared_ptr<GameObject>> children_;
  std::vector<std::unique_ptr<IComponent>> components_;
};

} // namespace AbsoluteEngine
