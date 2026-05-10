#include "LevelLoader.h"
#include <fstream>
#include <assert.h>
#include <externals/nlohmann/json.hpp>

using json = nlohmann::json;

// 定数
const std::string kDefaultBaseDirectory = "resources/levels/";
const std::string kExtension = ".json";

// 内部関数：再帰的にオブジェクトをパースする
void ParseRecursive(json& object, LevelData::ObjectData& parentData) {
    // 種類と名前
    parentData.type = object["type"].get<std::string>();
    parentData.name = object["name"].get<std::string>();

    // トランスフォームの取得
    json& transform = object["transform"];
    
    // Blender(Z-up) -> Game(Y-up) への変換
    // 座標: (X, Y, Z) -> (X, Z, -Y) ※Blenderの-Yをゲームの+Z(正面)に合わせる
    parentData.translation.x = (float)transform["translation"][0];
    parentData.translation.y = (float)transform["translation"][2];
    parentData.translation.z = -(float)transform["translation"][1];

    // 回転: (X, Y, Z) -> (-X, -Z, -Y) ※軸対応と符号の調整
    float toRad = 3.14159265f / 180.0f;
    parentData.rotation.x = -(float)transform["rotation"][0] * toRad;
    parentData.rotation.y = -(float)transform["rotation"][2] * toRad;
    parentData.rotation.z = -(float)transform["rotation"][1] * toRad;

    // スケーリング: (X, Y, Z) -> (X, Z, Y)
    parentData.scaling.x = (float)transform["scaling"][0];
    parentData.scaling.y = (float)transform["scaling"][2];
    parentData.scaling.z = (float)transform["scaling"][1];

    // カスタムプロパティ：file_name
    if (object.contains("file_name")) {
        parentData.fileName = object["file_name"].get<std::string>();
    }

    // カスタムプロパティ：collider
    if (object.contains("collider")) {
        LevelData::ColliderData collider;
        collider.type = object["collider"]["type"].get<std::string>();
        collider.center.x = (float)object["collider"]["center"][0];
        collider.center.y = (float)object["collider"]["center"][2];
        collider.center.z = (float)object["collider"]["center"][1];
        collider.size.x = (float)object["collider"]["size"][0];
        collider.size.y = (float)object["collider"]["size"][2];
        collider.size.z = (float)object["collider"]["size"][1];
        parentData.collider = collider;
    }

    // 無効オプションの取得
    if (object.contains("無効オプション")) {
        parentData.isDisabled = object["無効オプション"].get<bool>();
    }

    // 子要素の再帰的パース
    if (object.contains("children")) {
        for (auto& child : object["children"]) {
            parentData.children.emplace_back();
            ParseRecursive(child, parentData.children.back());
        }
    }
}

std::unique_ptr<LevelData> LevelLoader::Load(const std::string& fileName) {
    // フルパスの構築
    const std::string fullPath = kDefaultBaseDirectory + fileName + kExtension;

    // ファイルオープン
    std::ifstream file;
    file.open(fullPath);
    if (file.fail()) {
        assert(0 && "Failed to open level file.");
        return nullptr;
    }

    // JSONのデシリアライズ
    json deserialized;
    file >> deserialized;

    // 正しいレベルデータかチェック
    assert(deserialized.is_object());
    assert(deserialized.contains("name"));
    assert(deserialized["name"].get<std::string>() == "scene");

    // レベルデータの構築
    std::unique_ptr<LevelData> levelData = std::make_unique<LevelData>();

    // オブジェクトの走査
    for (auto& object : deserialized["objects"]) {
        levelData->objects.emplace_back();
        ParseRecursive(object, levelData->objects.back());
    }

    return levelData;
}
