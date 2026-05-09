#pragma once
#include "LevelData.h"
#include <string>
#include <memory>

class LevelLoader {
public:
    /// <summary>
    /// レベルデータを読み込む
    /// </summary>
    /// <param name="fileName">ファイル名（resources/ からの相対パス）</param>
    /// <returns>読み込まれたレベルデータ</returns>
    static std::unique_ptr<LevelData> Load(const std::string& fileName);

private:
    // ヘルパー関数などは必要に応じて追加
};
