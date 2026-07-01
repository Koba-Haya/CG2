#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include "../scene/GameObject.h"
#include "../scene/LightNodeComponent.h"
#include "../Type/Transform.h"
#include "../../Application/camera/RailCameraController.h"

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
    LightCommand(std::shared_ptr<GameObject> target, const LightNodeComponent& before, const LightNodeComponent& after)
        : target_(target), before_(before), after_(after) {}

    void Execute() override {
        if (auto t = target_.lock()) {
            if (auto comp = t->GetComponent<LightNodeComponent>()) {
                *comp = after_;
            }
        }
    }

    void Undo() override {
        if (auto t = target_.lock()) {
            if (auto comp = t->GetComponent<LightNodeComponent>()) {
                *comp = before_;
            }
        }
    }

private:
    std::weak_ptr<GameObject> target_;
    LightNodeComponent before_;
    LightNodeComponent after_;
};

// レールカメラのポイント変更コマンド
class RailCameraCommand : public ICommand {
public:
    // RailCameraControllerのポインタを直接保持すると破棄された際に危険だが、
    // エディタのライフサイクル上は基本的に生きている前提とする。
    RailCameraCommand(class RailCameraController* target, const std::vector<Vector3>& before, const std::vector<Vector3>& after)
        : target_(target), before_(before), after_(after) {}

    void Execute() override {
        if (target_) {
            target_->SetWaypoints(after_);
            // 変更フラグは立てないか、立てるか？
            // 履歴操作時も変更フラグを立ててオートセーブさせるのが自然。
            target_->SetModifiedFlag(); 
        }
    }

    void Undo() override {
        if (target_) {
            target_->SetWaypoints(before_);
            target_->SetModifiedFlag();
        }
    }

private:
    class RailCameraController* target_;
    std::vector<Vector3> before_;
    std::vector<Vector3> after_;
};

} // namespace AbsoluteEngine
