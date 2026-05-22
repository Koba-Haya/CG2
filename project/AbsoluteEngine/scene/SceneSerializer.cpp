#include "SceneSerializer.h"
#include <fstream>
#include <iostream>
#include "../../externals/nlohmann/json.hpp"

using json = nlohmann::json;

namespace AbsoluteEngine {

// Vector3 を JSON に変換するヘルパー
static json Vector3ToJson(const Vector3& v) {
  return json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
}

static Vector3 JsonToVector3(const json& j) {
  return { j.value("x", 0.0f), j.value("y", 0.0f), j.value("z", 0.0f) };
}

// GameObject 1つを JSON にシリアライズ
static json SerializeGameObject(const std::shared_ptr<GameObject>& obj) {
  if (!obj) return json();

  json j;
  j["name"] = obj->GetName();
  j["tag"] = obj->GetTag();
  
  const Transform& t = obj->GetTransform();
  j["transform"] = {
    {"translate", Vector3ToJson(t.translate)},
    {"rotate", Vector3ToJson(t.rotate)},
    {"scale", Vector3ToJson(t.scale)}
  };

  const ColliderInfo& c = obj->GetCollider();
  j["collider"] = {
    {"type", static_cast<int>(c.type)},
    {"centerOffset", Vector3ToJson(c.centerOffset)},
    {"radius", c.radius},
    {"size", Vector3ToJson(c.size)}
  };

  json childrenJson = json::array();
  for (const auto& child : obj->GetChildren()) {
    childrenJson.push_back(SerializeGameObject(child));
  }
  j["children"] = childrenJson;

  return j;
}

// JSON から GameObject をデシリアライズ
static std::shared_ptr<GameObject> DeserializeGameObject(const json& j) {
  std::shared_ptr<GameObject> obj = std::make_shared<GameObject>(j.value("name", "GameObject"));
  obj->SetTag(j.value("tag", "Untagged"));

  if (j.contains("transform")) {
    const auto& tJson = j["transform"];
    Transform& t = obj->GetTransform();
    t.translate = JsonToVector3(tJson["translate"]);
    t.rotate = JsonToVector3(tJson["rotate"]);
    t.scale = JsonToVector3(tJson["scale"]);
  }

  if (j.contains("collider")) {
    const auto& cJson = j["collider"];
    ColliderInfo& c = obj->GetCollider();
    c.type = static_cast<ColliderInfo::Type>(cJson.value("type", 0));
    c.centerOffset = JsonToVector3(cJson["centerOffset"]);
    c.radius = cJson.value("radius", 1.0f);
    c.size = JsonToVector3(cJson["size"]);
  }

  if (j.contains("children") && j["children"].is_array()) {
    for (const auto& childJson : j["children"]) {
      auto childObj = DeserializeGameObject(childJson);
      if (childObj) {
        obj->AddChild(childObj);
      }
    }
  }

  return obj;
}

bool SceneSerializer::Serialize(const std::string& filepath, const std::vector<std::shared_ptr<GameObject>>& rootObjects) {
  json j;
  json rootArray = json::array();
  for (const auto& obj : rootObjects) {
    rootArray.push_back(SerializeGameObject(obj));
  }
  j["rootObjects"] = rootArray;

  std::ofstream file(filepath);
  if (file.is_open()) {
    file << j.dump(4); // 4スペースインデントで出力
    return true;
  }
  return false;
}

bool SceneSerializer::Deserialize(const std::string& filepath, std::vector<std::shared_ptr<GameObject>>& outRootObjects) {
  std::ifstream file(filepath);
  if (!file.is_open()) return false;

  json j;
  try {
    file >> j;
  } catch (const json::parse_error& e) {
    std::cerr << "JSON parse error: " << e.what() << std::endl;
    return false;
  }

  outRootObjects.clear();
  if (j.contains("rootObjects") && j["rootObjects"].is_array()) {
    for (const auto& childJson : j["rootObjects"]) {
      auto obj = DeserializeGameObject(childJson);
      if (obj) {
        outRootObjects.push_back(obj);
      }
    }
  }

  return true;
}

} // namespace AbsoluteEngine
