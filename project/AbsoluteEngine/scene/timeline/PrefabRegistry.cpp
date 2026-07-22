// ============================================================
// PrefabRegistry.cpp
// プレハブディレクトリのスキャンとID解決の実装
// ============================================================
#include "PrefabRegistry.h"
#include "../../externals/nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace AbsoluteEngine {

void PrefabRegistry::ScanDirectory(const std::string& directoryPath) {
    // スキャン前に辞書をクリアしてから再構築する
    idToPath_.clear();

    if (!std::filesystem::exists(directoryPath)) {
        std::cerr << "[PrefabRegistry] ディレクトリが存在しません: " << directoryPath << std::endl;
        return;
    }

    // 指定ディレクトリ以下を再帰的にスキャン
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath)) {
        // .jsonファイルのみ対象
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }

        const std::string filePath = entry.path().string();

        // JSONをパースしてIDを取得
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[PrefabRegistry] ファイルを開けません: " << filePath << std::endl;
            continue;
        }

        json j;
        try {
            file >> j;
        } catch (const json::parse_error& e) {
            std::cerr << "[PrefabRegistry] JSONパースエラー: " << filePath
                      << " -> " << e.what() << std::endl;
            continue;
        }

        // "name" フィールドをIDとして使用
        if (!j.contains("name") || !j["name"].is_string()) {
            std::cerr << "[PrefabRegistry] 'name'フィールドが見つかりません: " << filePath << std::endl;
            continue;
        }

        const std::string prefabId = j["name"].get<std::string>();

        // IDの重複チェック（エラーハンドリング）
        if (idToPath_.count(prefabId) > 0) {
            std::cerr << "[PrefabRegistry] 警告: プレハブIDが重複しています! ID=\"" << prefabId
                      << "\"  既存パス=\"" << idToPath_[prefabId]
                      << "\"  新規パス=\"" << filePath << "\" -> 既存を優先します" << std::endl;
            continue;
        }

        idToPath_[prefabId] = filePath;
        std::cout << "[PrefabRegistry] 登録: \"" << prefabId << "\" <- " << filePath << std::endl;
    }

    std::cout << "[PrefabRegistry] スキャン完了。登録件数: " << idToPath_.size() << std::endl;
}

std::string PrefabRegistry::ResolveId(const std::string& prefabId) const {
    const auto it = idToPath_.find(prefabId);
    if (it == idToPath_.end()) {
        std::cerr << "[PrefabRegistry] 警告: プレハブIDが見つかりません: \"" << prefabId << "\"" << std::endl;
        return "";
    }
    return it->second;
}

std::vector<std::string> PrefabRegistry::GetAllIds() const {
    std::vector<std::string> ids;
    ids.reserve(idToPath_.size());
    for (const auto& pair : idToPath_) {
        ids.push_back(pair.first);
    }
    // アルファベット順にソートしてUIのドロップダウンを見やすくする
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace AbsoluteEngine
