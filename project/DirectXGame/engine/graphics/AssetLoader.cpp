#include "AssetLoader.h"
#include <cassert>
#include <fstream>
#include <sstream>

const ModelData &AssetLoader::LoadObj(const std::string &dir,
                                      const std::string &file) {
  std::string key = dir + "/" + file;
  auto it = modelCache_.find(key);
  if (it != modelCache_.end())
    return it->second;
  return modelCache_.emplace(key, LoadObjImpl(dir, file)).first->second;
}

const MaterialData &AssetLoader::LoadMtl(const std::string &dir,
                                         const std::string &file) {
  std::string key = dir + "/" + file;
  auto it = materialCache_.find(key);
  if (it != materialCache_.end())
    return it->second;
  return materialCache_.emplace(key, LoadMtlImpl(dir, file)).first->second;
}

// ---- ここは今 main.cpp にある実装をそのまま移植 ----
ModelData AssetLoader::LoadObjImpl(const std::string &directoryPath,
                                   const std::string &filename) {
  ModelData modelData;
  std::vector<Vector4> positions;
  std::vector<Vector3> normals;
  std::vector<Vector2> texcoords;
  std::string line;

  std::ifstream file(directoryPath + "/" + filename);
  assert(file.is_open());

  while (std::getline(file, line)) {
    std::string id;
    std::istringstream s(line);
    s >> id;
    if (id == "v") {
      Vector4 p;
      s >> p.x >> p.y >> p.z;
      p.w = 1.0f;
      positions.push_back(p);
    } else if (id == "vt") {
      Vector2 t;
      s >> t.x >> t.y;
      texcoords.push_back(t);
    } else if (id == "vn") {
      Vector3 n;
      s >> n.x >> n.y >> n.z;
      normals.push_back(n);
    } else if (id == "f") {
      VertexData tri[3];
      for (int i = 0; i < 3; i++) {
        std::string def;
        s >> def;
        std::istringstream v(def);
        uint32_t idx[3];
        for (int e = 0; e < 3; e++) {
          std::string tmp;
          std::getline(v, tmp, '/');
          idx[e] = std::stoi(tmp);
        }
        Vector4 p = positions[idx[0] - 1];
        Vector2 t = texcoords[idx[1] - 1];
        Vector3 n = normals[idx[2] - 1];
        p.x *= -1.0f;
        n.x *= -1.0f;
        t.y = 1.0f - t.y;
        tri[i] = {p, t, n};
      }
      modelData.vertices.push_back(tri[2]);
      modelData.vertices.push_back(tri[1]);
      modelData.vertices.push_back(tri[0]);
    } else if (id == "mtllib") {
      std::string mtl;
      s >> mtl;
      modelData.material = LoadMtl(directoryPath, mtl); // キャッシュ経由
    }
  }
  return modelData;
}

MaterialData AssetLoader::LoadMtlImpl(const std::string &directoryPath,
                                      const std::string &filename) {
  MaterialData m;
  std::string line;
  std::ifstream file(directoryPath + "/" + filename);
  assert(file.is_open());
  while (std::getline(file, line)) {
    std::string id;
    std::istringstream s(line);
    s >> id;
    if (id == "map_Kd") {
      std::string tex;
      s >> tex;
      m.textureFilePath = directoryPath + "/" + tex;
    }
  }
  return m;
}
