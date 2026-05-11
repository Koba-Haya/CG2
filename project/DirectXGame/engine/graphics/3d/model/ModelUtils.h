#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Matrix.h"
#include "Method.h"
#include "Vector.h"
#include "Transform.h"

// ===== CPU側のモデルデータ（OBJ/MTL読み込み結果） =====

struct VertexData {
  Vector4 position{};
  Vector2 texcoord{};
  Vector3 normal{};
  float pad[3] = {0.0f, 0.0f, 0.0f}; // Ensure 48 bytes (multiple of 16)
};

struct VertexBoneData {
  uint32_t boneIDs[4] = {0, 0, 0, 0};
  float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct MaterialData {
  std::string textureFilePath;
};

struct MeshData {
  std::vector<VertexData> vertices;
  std::vector<VertexBoneData> skinningData;
  int materialIndex = -1;
};

struct Node {
  QuaternionTransform transform;
  Matrix4x4 localMatrix;
  std::string name;
  std::vector<uint32_t> meshIndices;
  std::vector<Node> children;
};

struct Joint {
  QuaternionTransform transform;
  Matrix4x4 localMatrix;
  Matrix4x4 skeletonSpaceMatrix;
  std::string name;
  std::vector<int32_t> children; // 資料に合わせてchildrenIndicesからchildrenにリネーム
  int32_t index;
  Matrix4x4 inverseBindPoseMatrix;
};

struct Skeleton {
  int32_t rootJointIndex = -1;
  std::map<std::string, int32_t> jointMap;
  std::vector<Joint> joints;
};

struct ModelData {
  std::vector<MeshData> meshes;
  std::vector<MaterialData> materials;
  Node rootNode;
  Skeleton skeleton;
};

Skeleton CreateSkeleton(const Node& rootNode);
void UpdateSkeleton(Skeleton& skeleton);

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
std::vector<VertexBoneData> FlattenSkinningData(const ModelData &model);
std::string PickDiffuseTexturePath(const ModelData &model);
