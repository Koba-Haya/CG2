// ============================================================
// PrefabRegistry.h
// 役割: プレハブID -> ファイルパス の変換辞書を管理するシングルトン
//       プレハブのファイル名が変わってもIDで参照できる堅牢性を提供する
// ============================================================
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace AbsoluteEngine {

class PrefabRegistry {
public:
    // シングルトンインスタンス取得
    static PrefabRegistry& GetInstance() {
        static PrefabRegistry instance;
        return instance;
    }

    // 指定ディレクトリ以下のプレハブJSONを再帰スキャンし、辞書を構築する
    // スキャンは初期化時・エディタリロード時のみ実行すること
    void ScanDirectory(const std::string& directoryPath);

    // IDからファイルパスを解決する
    // 見つからない場合は空文字列を返す
    std::string ResolveId(const std::string& prefabId) const;

    // 登録済みIDの一覧を取得（エディタUIのドロップダウン用）
    std::vector<std::string> GetAllIds() const;

    // 登録件数を取得
    size_t GetCount() const { return idToPath_.size(); }

    // 辞書をクリアして再スキャン可能な状態にする
    void Clear() { idToPath_.clear(); }

private:
    // シングルトンのため外部からの生成を禁止
    PrefabRegistry() = default;
    ~PrefabRegistry() = default;
    PrefabRegistry(const PrefabRegistry&) = delete;
    PrefabRegistry& operator=(const PrefabRegistry&) = delete;

private:
    // キー: プレハブID（JSONの "name" フィールド）
    // 値:   プレハブJSONファイルの絶対パス
    std::unordered_map<std::string, std::string> idToPath_;
};

} // namespace AbsoluteEngine
