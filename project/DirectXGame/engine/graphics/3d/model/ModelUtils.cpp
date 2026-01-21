#include "ModelUtils.h"
#include <utility>

Matrix4x4 ConvertAssimpMatrix(const aiMatrix4x4 &a) {
  Matrix4x4 m{};
  m.m[0][0] = a[0][0];
  m.m[0][1] = a[0][1];
  m.m[0][2] = a[0][2];
  m.m[0][3] = a[0][3];
  m.m[1][0] = a[1][0];
  m.m[1][1] = a[1][1];
  m.m[1][2] = a[1][2];
  m.m[1][3] = a[1][3];
  m.m[2][0] = a[2][0];
  m.m[2][1] = a[2][1];
  m.m[2][2] = a[2][2];
  m.m[2][3] = a[2][3];
  m.m[3][0] = a[3][0];
  m.m[3][1] = a[3][1];
  m.m[3][2] = a[3][2];
  m.m[3][3] = a[3][3];
  return m;
}

Matrix4x4 ConvertAssimpMatrixTransposed(const aiMatrix4x4 &a) {
  Matrix4x4 m = ConvertAssimpMatrix(a);
  return Transpose(m);
}

VertexData FixupVertex_AssimpToEngine(const VertexData &v,
                                      const UVFixupOptions &opt) {
  VertexData r = v;

  if (opt.flipU) {
    r.texcoord.x = 1.0f - r.texcoord.x;
  }
  if (opt.flipV) {
    r.texcoord.y = 1.0f - r.texcoord.y;
  }

  return r;
}

void FlipTriangleWinding(VertexData &a, VertexData &b, VertexData &c) {
  std::swap(b, c);
}

std::vector<VertexData> FlattenVertices(const ModelData &model) {
  std::vector<VertexData> out;
  size_t total = 0;
  for (const auto &m : model.meshes) {
    total += m.vertices.size();
  }
  out.reserve(total);

  for (const auto &m : model.meshes) {
    out.insert(out.end(), m.vertices.begin(), m.vertices.end());
  }
  return out;
}

std::string PickDiffuseTexturePath(const ModelData &model) {
  for (const auto &mat : model.materials) {
    if (!mat.textureFilePath.empty()) {
      return mat.textureFilePath;
    }
  }
  return {};
}
