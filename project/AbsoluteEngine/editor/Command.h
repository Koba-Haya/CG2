#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include "../scene/GameObject.h"
#include "../Type/Transform.h"

namespace AbsoluteEngine {

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0; // Redo時または初回実行時
    virtual void Undo() = 0;    // Undo時
};

// Transform変更のコマンド
class TransformCommand : public ICommand {
public:
    TransformCommand(std::shared_ptr<GameObject> target, const Transform& before, const Transform& after)
        : target_(target), before_(before), after_(after) {}

    void Execute() override {
        if (auto t = target_.lock()) {
            t->GetTransform() = after_;
        }
    }

    void Undo() override {
        if (auto t = target_.lock()) {
            t->GetTransform() = before_;
        }
    }

private:
    std::weak_ptr<GameObject> target_;
    Transform before_;
    Transform after_;
};

// オブジェクト生成のコマンド
class CreateObjectCommand : public ICommand {
public:
    CreateObjectCommand(std::shared_ptr<GameObject> target, std::vector<std::shared_ptr<GameObject>>* rootObjects)
        : target_(target), rootObjects_(rootObjects) {}

    CreateObjectCommand(std::shared_ptr<GameObject> target, std::shared_ptr<GameObject> parent)
        : target_(target), parent_(parent), rootObjects_(nullptr) {}

    void Execute() override {
        if (auto p = parent_.lock()) {
            p->AddChild(target_);
        } else if (rootObjects_) {
            rootObjects_->push_back(target_);
        }
    }

    void Undo() override {
        if (auto p = parent_.lock()) {
            p->RemoveChild(target_);
        } else if (rootObjects_) {
            auto it = std::find(rootObjects_->begin(), rootObjects_->end(), target_);
            if (it != rootObjects_->end()) {
                rootObjects_->erase(it);
            }
        }
    }

private:
    std::shared_ptr<GameObject> target_;
    std::weak_ptr<GameObject> parent_;
    std::vector<std::shared_ptr<GameObject>>* rootObjects_;
};

// オブジェクト削除のコマンド
class DeleteObjectCommand : public ICommand {
public:
    DeleteObjectCommand(std::shared_ptr<GameObject> target, std::vector<std::shared_ptr<GameObject>>* rootObjects)
        : target_(target), rootObjects_(rootObjects) {}

    DeleteObjectCommand(std::shared_ptr<GameObject> target, std::shared_ptr<GameObject> parent)
        : target_(target), parent_(parent), rootObjects_(nullptr) {}

    void Execute() override {
        if (auto p = parent_.lock()) {
            p->RemoveChild(target_);
        } else if (rootObjects_) {
            auto it = std::find(rootObjects_->begin(), rootObjects_->end(), target_);
            if (it != rootObjects_->end()) {
                rootObjects_->erase(it);
            }
        }
    }

    void Undo() override {
        if (auto p = parent_.lock()) {
            p->AddChild(target_);
        } else if (rootObjects_) {
            rootObjects_->push_back(target_);
        }
    }

private:
    std::shared_ptr<GameObject> target_;
    std::weak_ptr<GameObject> parent_;
    std::vector<std::shared_ptr<GameObject>>* rootObjects_;
};

// ライト変更のコマンド
class LightCommand : public ICommand {
public:
    LightCommand(std::shared_ptr<GameObject> target, const LightComponent& before, const LightComponent& after)
        : target_(target), before_(before), after_(after) {}

    void Execute() override {
        if (auto t = target_.lock()) {
            t->GetLight() = after_;
        }
    }

    void Undo() override {
        if (auto t = target_.lock()) {
            t->GetLight() = before_;
        }
    }

private:
    std::weak_ptr<GameObject> target_;
    LightComponent before_;
    LightComponent after_;
};

} // namespace AbsoluteEngine
