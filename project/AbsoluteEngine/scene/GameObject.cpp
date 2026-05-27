#include "GameObject.h"
#include <algorithm>
#include "../graphics/3d/model/ModelManager.h"
#include "../graphics/texture/TextureManager.h"

namespace AbsoluteEngine {

GameObject::GameObject(const std::string& name) : name_(name) {
}

void GameObject::AddChild(std::shared_ptr<GameObject> child) {
  if (child) {
    child->parent_ = shared_from_this();
    children_.push_back(child);
  }
}

void GameObject::RemoveChild(std::shared_ptr<GameObject> child) {
  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end()) {
    (*it)->parent_.reset();
    children_.erase(it);
  }
}

void GameObject::AddComponent(std::unique_ptr<IComponent> component) {
  if (component) {
    component->SetOwner(this);
    components_.push_back(std::move(component));
  }
}

void GameObject::RemoveComponent(IComponent* component) {
  auto it = std::remove_if(components_.begin(), components_.end(),
      [component](const std::unique_ptr<IComponent>& ptr) {
          return ptr.get() == component;
      });
  if (it != components_.end()) {
      components_.erase(it, components_.end());
  }
}

void GameObject::Update(float deltaTime) {
  for (auto& comp : components_) {
    comp->Update(deltaTime);
  }
  for (auto& child : children_) {
    child->Update(deltaTime);
  }
}

void GameObject::Draw() {
  if (modelInstance_) {
    // 自身のTransformからWorld行列を計算してモデルに渡す
    Matrix4x4 world = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    // もし親の行列も考慮するならここで乗算する（今回は簡易的にローカルのみか、親を計算するか要検討。現状は階層計算なしの仕様に見える）
    
    // もし親がいれば、親のワールド行列を乗算するべきだが、現状 Transform がシンプルなので簡易的に自身のみ
    if (auto p = parent_.lock()) {
      // 本来は再帰的にワールド行列を計算すべき。ここでは簡易対応として親のTransformを加算/乗算
      // ここでは複雑になるため、Transformクラス側にCalculateWorldMatrix等が必要。
      // とりあえず今回はローカルTransformのみを反映。
    }

    modelInstance_->SetWorld(world);
    modelInstance_->SetDissolveParam(dissolve_.enable, dissolve_.threshold, dissolve_.edgeRange, dissolve_.edgeColor, dissolve_.maskColor);
    modelInstance_->Draw();
  }

  // 子のDrawを呼ぶ
  for (auto& child : children_) {
    child->Draw();
  }
}

void GameObject::LoadModel(const std::string& path) {
  auto res = ModelManager::GetInstance()->Load(path);
  if (!res) return; // ロードに失敗した場合は何もしない

  modelPath_ = path;
  modelInstance_ = std::make_unique<ModelInstance>();
  ModelInstance::CreateInfo ci{};
  ci.resource = res;
  ci.baseColor = {1, 1, 1, 1};
  ci.lightingMode = 1;
  modelInstance_->Initialize(ci);
  
  if (!texturePath_.empty()) {
    LoadTexture(texturePath_);
  }
}

void GameObject::LoadTexture(const std::string& path) {
  texturePath_ = path;
  if (modelInstance_) {
    auto tex = TextureManager::GetInstance()->Load(path);
    if (tex) {
      modelInstance_->SetOverrideTexture(tex);
    }
  }
}

} // namespace AbsoluteEngine
