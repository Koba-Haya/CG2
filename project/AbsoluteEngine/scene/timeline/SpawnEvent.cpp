// ============================================================
// SpawnEvent.cpp
// プレハブスポーンイベントの実装
// ============================================================
#include "SpawnEvent.h"
#include "PrefabRegistry.h"
#include "../SceneSerializer.h"
#include "../BaseScene.h"
#include "../GameObject.h"
#include <iostream>

// JSON変換用ヘルパー（同一ファイル内ローカル定義）
static nlohmann::json TransformToJson(const Transform& t) {
    return {
        {"translate", {{"x", t.translate.x}, {"y", t.translate.y}, {"z", t.translate.z}}},
        {"rotate",    {{"x", t.rotate.x},    {"y", t.rotate.y},    {"z", t.rotate.z}}},
        {"scale",     {{"x", t.scale.x},     {"y", t.scale.y},     {"z", t.scale.z}}}
    };
}

static Transform JsonToTransform(const nlohmann::json& j) {
    Transform t{};
    if (j.contains("translate")) {
        t.translate.x = j["translate"].value("x", 0.0f);
        t.translate.y = j["translate"].value("y", 0.0f);
        t.translate.z = j["translate"].value("z", 0.0f);
    }
    if (j.contains("rotate")) {
        t.rotate.x = j["rotate"].value("x", 0.0f);
        t.rotate.y = j["rotate"].value("y", 0.0f);
        t.rotate.z = j["rotate"].value("z", 0.0f);
    }
    if (j.contains("scale")) {
        t.scale.x = j["scale"].value("x", 1.0f);
        t.scale.y = j["scale"].value("y", 1.0f);
        t.scale.z = j["scale"].value("z", 1.0f);
    }
    return t;
}

namespace AbsoluteEngine {

std::string SpawnEvent::GetLabel() const {
    return "Spawn: " + (prefabId_.empty() ? "(未設定)" : prefabId_);
}

void SpawnEvent::Fire() {
    // 既にスポーン済みのオブジェクトが存在する場合はスキップ（二重スポーン防止）
    if (!spawnedObject_.expired()) {
        return;
    }

    // PrefabRegistryからIDでファイルパスを解決する
    const std::string path = PrefabRegistry::GetInstance().ResolveId(prefabId_);
    if (path.empty()) {
        std::cerr << "[SpawnEvent] プレハブIDを解決できません: \"" << prefabId_ << "\"" << std::endl;
        return;
    }

    // プレハブをロードしてオブジェクトを生成する
    auto obj = SceneSerializer::LoadPrefab(path);
    if (!obj) {
        std::cerr << "[SpawnEvent] プレハブのロードに失敗しました: " << path << std::endl;
        return;
    }

    // スポーン位置に配置する
    obj->GetTransform() = spawnTransform_;

    // アクティブシーンのルートオブジェクトに追加する
    BaseScene* activeScene = BaseScene::GetActiveScene();
    if (!activeScene) {
        std::cerr << "[SpawnEvent] アクティブシーンが存在しません" << std::endl;
        return;
    }

    // shared_ptr のまま rootObjects に追加し、weak_ptr で参照を保持する
    activeScene->AddRootObject(obj);
    spawnedObject_ = obj;

    std::cout << "[SpawnEvent] スポーン完了: \"" << prefabId_ << "\"" << std::endl;
}

void SpawnEvent::Rewind() {
    // weak_ptr でオブジェクトの生存を確認してから破棄する
    // （expired() が true の場合はすでに別の手段で破棄されているので何もしない）
    if (auto ptr = spawnedObject_.lock()) {
        ptr->Destroy();
        std::cout << "[SpawnEvent] Rewind: \"" << prefabId_ << "\" を破棄しました" << std::endl;
    }

    // weak_ptr をリセットして次のFire()に備える
    spawnedObject_.reset();
}

void SpawnEvent::Serialize(nlohmann::json& j) const {
    j["prefabId"] = prefabId_;
    j["spawnTransform"] = TransformToJson(spawnTransform_);
}

void SpawnEvent::Deserialize(const nlohmann::json& j) {
    prefabId_ = j.value("prefabId", "");
    if (j.contains("spawnTransform")) {
        spawnTransform_ = JsonToTransform(j["spawnTransform"]);
    }
}

} // namespace AbsoluteEngine
