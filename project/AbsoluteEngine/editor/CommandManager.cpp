#include "CommandManager.h"

namespace AbsoluteEngine {

void CommandManager::ExecuteCommand(std::shared_ptr<ICommand> command) {
    command->Execute();
    AddCommand(command);
}

void CommandManager::AddCommand(std::shared_ptr<ICommand> command) {
    // 現在のインデックスより後の履歴は破棄する（Undo後に新しい操作をした場合）
    if (currentIndex_ < static_cast<int>(history_.size()) - 1) {
        history_.erase(history_.begin() + currentIndex_ + 1, history_.end());
    }
    
    history_.push_back(command);
    currentIndex_++;
}

void CommandManager::Undo() {
    if (currentIndex_ >= 0) {
        history_[currentIndex_]->Undo();
        currentIndex_--;
    }
}

void CommandManager::Redo() {
    if (currentIndex_ < static_cast<int>(history_.size()) - 1) {
        currentIndex_++;
        history_[currentIndex_]->Execute();
    }
}

void CommandManager::Clear() {
    history_.clear();
    currentIndex_ = -1;
}

} // namespace AbsoluteEngine
