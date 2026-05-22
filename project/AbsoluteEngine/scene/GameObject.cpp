#include "GameObject.h"
#include <algorithm>

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
  if (child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
      (*it)->parent_.reset();
      children_.erase(it);
    }
  }
}

void GameObject::Update(float deltaTime) {
  // 子のUpdateを呼ぶ
  for (auto& child : children_) {
    child->Update(deltaTime);
  }
}

} // namespace AbsoluteEngine
