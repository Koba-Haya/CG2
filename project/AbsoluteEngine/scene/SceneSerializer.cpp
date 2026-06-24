#include "SceneSerializer.h"
#include <fstream>
#include <iostream>
#include "../../externals/nlohmann/json.hpp"
#include "ComponentFactory.h"
#include "ColliderComponent.h"
#include "LightNodeComponent.h"
#include "ModelComponent.h"
#include "DissolveComponent.h"

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
static json SerializeGameObject(const std::shared_ptr<GameObject>& obj, bool forceFullSerialize = false) {
  if (!obj) return json();

  json j;

  if (!forceFullSerialize && obj->IsPrefabInstance()) {
    j["prefabPath"] = obj->GetPrefabPath();
    const Transform& t = obj->GetTransform();
    j["transform"] = {
      {"translate", Vector3ToJson(t.translate)},
      {"rotate", Vector3ToJson(t.rotate)},
      {"scale", Vector3ToJson(t.scale)}
    };
    return j;
  }

  j["name"] = obj->GetName();
  j["tag"] = obj->GetTag();
  auto modelComp = obj->GetComponent<ModelComponent>();
  if (modelComp) {
    j["modelPath"] = modelComp->GetModelPath();
    j["texturePath"] = modelComp->GetTexturePath();
  }
  
  const Transform& t = obj->GetTransform();
  j["transform"] = {
    {"translate", Vector3ToJson(t.translate)},
    {"rotate", Vector3ToJson(t.rotate)},
    {"scale", Vector3ToJson(t.scale)}
  };

  auto colliderComp = obj->GetComponent<ColliderComponent>();
  if (colliderComp) {
    const ColliderComponent& c = *colliderComp;
    j["collider"] = {
      {"type", static_cast<int>(c.type)},
      {"centerOffset", Vector3ToJson(c.centerOffset)},
      {"radius", c.radius},
      {"size", Vector3ToJson(c.size)}
    };
  }

  auto lightComp = obj->GetComponent<LightNodeComponent>();
  if (lightComp && lightComp->type != LightNodeComponent::Type::None) {
    const LightNodeComponent& l = *lightComp;
    j["light"] = {
      {"type", static_cast<int>(l.type)},
      {"color", Vector3ToJson(l.color)},
      {"intensity", l.intensity},
      {"radius", l.radius},
      {"decay", l.decay},
      {"distance", l.distance},
      {"coneAngleDeg", l.coneAngleDeg}
    };
  }

  json componentsJson = json::array();
  for (const auto& comp : obj->GetComponents()) {
    json cJson;
    cJson["type"] = comp->GetTypeName();
    componentsJson.push_back(cJson);
  }
  j["components"] = componentsJson;

  json childrenJson = json::array();
  for (const auto& child : obj->GetChildren()) {
    childrenJson.push_back(SerializeGameObject(child, forceFullSerialize));
  }
  j["children"] = childrenJson;

  return j;
}

// JSON から GameObject をデシリアライズ
static std::shared_ptr<GameObject> DeserializeGameObject(const json& j) {
  if (j.contains("prefabPath")) {
    std::string prefabPath = j["prefabPath"];
    auto obj = SceneSerializer::LoadPrefab(prefabPath);
    if (obj) {
      if (j.contains("transform")) {
        const auto& tJson = j["transform"];
        Transform& t = obj->GetTransform();
        t.translate = JsonToVector3(tJson["translate"]);
        t.rotate = JsonToVector3(tJson["rotate"]);
        t.scale = JsonToVector3(tJson["scale"]);
      }
      return obj;
    } else {
      std::cerr << "Failed to load prefab: " << prefabPath << std::endl;
      auto missingObj = std::make_shared<GameObject>("Missing Prefab");
      return missingObj;
    }
  }

  std::shared_ptr<GameObject> obj = std::make_shared<GameObject>(j.value("name", "GameObject"));
  obj->SetTag(j.value("tag", "Untagged"));

  std::string modelPath = j.value("modelPath", "");
  std::string texturePath = j.value("texturePath", "");
  if (!modelPath.empty() || !texturePath.empty()) {
    auto modelComp = std::make_unique<ModelComponent>();
    if (!modelPath.empty()) modelComp->LoadModel(modelPath);
    if (!texturePath.empty()) modelComp->LoadTexture(texturePath);
    obj->AddComponent(std::move(modelComp));
  }

  if (j.contains("transform")) {
    const auto& tJson = j["transform"];
    Transform& t = obj->GetTransform();
    t.translate = JsonToVector3(tJson["translate"]);
    t.rotate = JsonToVector3(tJson["rotate"]);
    t.scale = JsonToVector3(tJson["scale"]);
  }

  if (j.contains("collider")) {
    const auto& cJson = j["collider"];
    auto colliderComp = std::make_unique<ColliderComponent>();
    colliderComp->type = static_cast<ColliderComponent::Type>(cJson.value("type", 1));
    if (cJson.contains("centerOffset")) colliderComp->centerOffset = JsonToVector3(cJson["centerOffset"]);
    colliderComp->radius = cJson.value("radius", 1.0f);
    if (cJson.contains("size")) colliderComp->size = JsonToVector3(cJson["size"]);
    obj->AddComponent(std::move(colliderComp));
  }

  if (j.contains("light")) {
    const auto& lJson = j["light"];
    auto lightComp = std::make_unique<LightNodeComponent>();
    lightComp->type = static_cast<LightNodeComponent::Type>(lJson.value("type", 0));
    if (lJson.contains("color")) lightComp->color = JsonToVector3(lJson["color"]);
    lightComp->intensity = lJson.value("intensity", 1.0f);
    lightComp->radius = lJson.value("radius", 10.0f);
    lightComp->decay = lJson.value("decay", 2.0f);
    lightComp->distance = lJson.value("distance", 10.0f);
    lightComp->coneAngleDeg = lJson.value("coneAngleDeg", 30.0f);
    obj->AddComponent(std::move(lightComp));
  }

  if (j.contains("components") && j["components"].is_array()) {
    for (const auto& cJson : j["components"]) {
      std::string typeName = cJson.value("type", "");
      auto comp = ComponentFactory::GetInstance().Create(typeName);
      if (comp) {
        obj->AddComponent(std::move(comp));
      }
    }
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

std::string SceneSerializer::SerializeToString(const std::vector<std::shared_ptr<GameObject>>& rootObjects) {
  json j = json::array();
  for (const auto& obj : rootObjects) {
    j.push_back(SerializeGameObject(obj));
  }
  return j.dump(4);
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

std::shared_ptr<GameObject> SceneSerializer::CopyGameObject(std::shared_ptr<GameObject> src) {
  if (!src) return nullptr;
  json j = SerializeGameObject(src, true); // コピー時はフルシリアライズ
  return DeserializeGameObject(j);
}

bool SceneSerializer::DeserializeFromString(const std::string& jsonString, std::vector<std::shared_ptr<GameObject>>& outRootObjects) {
  if (jsonString.empty()) return false;
  try {
    json j = json::parse(jsonString);
    if (j.is_array()) {
      for (const auto& objJson : j) {
        auto obj = DeserializeGameObject(objJson);
        if (obj) {
          outRootObjects.push_back(obj);
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse json string: " << e.what() << std::endl;
    return false;
  }
  return true;
}

bool SceneSerializer::SavePrefab(const std::string& filepath, std::shared_ptr<GameObject> obj) {
  if (!obj) return false;
  
  json j = SerializeGameObject(obj, true);

  std::ofstream file(filepath);
  if (file.is_open()) {
    file << j.dump(4);
    return true;
  }
  return false;
}

std::shared_ptr<GameObject> SceneSerializer::LoadPrefab(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) return nullptr;

  json j;
  try {
    file >> j;
  } catch (const json::parse_error& e) {
    std::cerr << "JSON parse error: " << e.what() << std::endl;
    return nullptr;
  }

  auto obj = DeserializeGameObject(j);
  if (obj) {
    obj->SetPrefabPath(filepath);
  }
  return obj;
}

} // namespace AbsoluteEngine
