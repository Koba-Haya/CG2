#pragma once
#include "GameObject.h"
#include <string>
#include <vector>

namespace AbsoluteEngine {

class SceneSerializer {
public:
  // 現在のシーンツリーを JSON 形式で保存
  static bool Serialize(const std::string& filepath, const std::vector<std::shared_ptr<GameObject>>& rootObjects);

  // JSON からシーンツリーを読み込み
  static bool Deserialize(const std::string& filepath, std::vector<std::shared_ptr<GameObject>>& outRootObjects);
};

} // namespace AbsoluteEngine
