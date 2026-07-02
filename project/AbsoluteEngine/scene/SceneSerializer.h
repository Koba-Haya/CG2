#pragma once
#include "GameObject.h"
#include <string>
#include <vector>



namespace AbsoluteEngine {

class SceneSerializer {
public:
  // 現在のシーンツリーを JSON 形式で保存
  static bool Serialize(const std::string& filepath, const std::vector<std::shared_ptr<GameObject>>& rootObjects, bool forceFullSerialize = false);
  static std::string SerializeToString(const std::vector<std::shared_ptr<GameObject>>& rootObjects, bool forceFullSerialize = false);

  // JSON からシーンツリーを読み込み
  static bool Deserialize(const std::string& filepath, std::vector<std::shared_ptr<GameObject>>& outRootObjects);
  static bool DeserializeFromString(const std::string& jsonString, std::vector<std::shared_ptr<GameObject>>& outRootObjects);

  // プレハブの保存と読み込み
  static bool SavePrefab(const std::string& filepath, std::shared_ptr<GameObject> obj);
  static std::shared_ptr<GameObject> LoadPrefab(const std::string& filepath);

  // オブジェクトのディープコピーを作成（シリアライズの仕組みを利用）
  static std::shared_ptr<GameObject> CopyGameObject(std::shared_ptr<GameObject> src);
};

} // namespace AbsoluteEngine
