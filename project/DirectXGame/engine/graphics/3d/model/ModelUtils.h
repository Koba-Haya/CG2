#pragma once

#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Matrix.h"
#include "Method.h"
#include "Vector.h"

// ===== CPU側のモデルデータ（OBJ/MTL読み込み結果） =====

struct VertexData {
  Vector4 position{};
  Vector2 texcoord{};
  Vector3 normal{};
};

struct MaterialData {
  std::string textureFilePath;
};

struct MeshData {
  std::vector<VertexData> vertices;
  int materialIndex = -1;
};

struct Node {
  Matrix4x4 localMatrix{};
  std::string name;
  std::vector<uint32_t> meshIndices;
  std::vector<Node> children;
};

struct ModelData {
  std::vector<MeshData> meshes;
  std::vector<MaterialData> materials;
  Node rootNode;
};

Matrix4x4 ConvertAssimpMatrix(const aiMatrix4x4 &a);
Matrix4x4 ConvertAssimpMatrixTransposed(const aiMatrix4x4 &a);

struct UVFixupOptions {
  bool flipU = false;
  bool flipV = false;
};

VertexData FixupVertex_AssimpToEngine(const VertexData &v,
                                      const UVFixupOptions &opt);

void FlipTriangleWinding(VertexData &a, VertexData &b, VertexData &c);

std::vector<VertexData> FlattenVertices(const ModelData &model);
std::string PickDiffuseTexturePath(const ModelData &model);
