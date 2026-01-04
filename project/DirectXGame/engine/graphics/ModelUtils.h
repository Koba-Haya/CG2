#pragma once

#include <string>
#include <vector>

#include "Vector.h"

// ===== CPU側のモデルデータ（OBJ/MTL読み込み結果） =====

// OBJ の頂点1つ分
struct VertexData {
	Vector4 position{};
	Vector2 texcoord{};
	Vector3 normal{};
};

// MTL の最低限（map_Kd だけ）
struct MaterialData {
	std::string textureFilePath; // "dir/xxx.png" のようなパス（相対でもOK）
};

// OBJ の読み込み結果
struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

// OBJ / MTL ローダ
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
