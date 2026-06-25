#pragma once
#include "Command.h"
#include <vector>
#include <memory>
#include <functional>

namespace AbsoluteEngine {

class CommandManager {
public:
    CommandManager() = default;
    ~CommandManager() = default;

    // コマンド実行時のコールバック
    void SetOnCommandExecutedCallback(std::function<void()> callback) { onCommandExecuted_ = callback; }

    // コマンドを実行し、履歴に追加する（Undo後の場合はそれ以降の履歴を破棄する）
    void ExecuteCommand(std::shared_ptr<ICommand> command);
    
    // コマンドを履歴に追加するだけ（すでに操作自体は完了している場合に使用）
    void AddCommand(std::shared_ptr<ICommand> command);

    void Undo();
    void Redo();

    void Clear();

private:
    std::vector<std::shared_ptr<ICommand>> history_;
    int currentIndex_ = -1; // 最後に実行したコマンドのインデックス
    std::function<void()> onCommandExecuted_;
};

} // namespace AbsoluteEngine
