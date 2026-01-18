#include "ModelUtils.h"

#include <Windows.h>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static void LogFileOpenError_(const std::string &path) {
  std::string msg = "[ModelUtils] Failed to open file: " + path + "\n";
  OutputDebugStringA(msg.c_str());
}

ModelData LoadObjFile(const std::string &directoryPath,
                      const std::string &filename) {
  ModelData modelData;
  std::vector<Vector4> positions;
  std::vector<Vector3> normals;
  std::vector<Vector2> texcoords;
  std::string line;

  const std::string fullPath = directoryPath + "/" + filename;
  std::ifstream file(fullPath);

  if (!file.is_open()) {
    LogFileOpenError_(fullPath);
    return modelData; // vertices が空で返る（呼び出し側で検出して止める）
  }

  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier;

    if (identifier == "v") {
      Vector4 position;
      s >> position.x >> position.y >> position.z;
      position.w = 1.0f;
      positions.push_back(position);
    } else if (identifier == "vt") {
      Vector2 texcoord;
      s >> texcoord.x >> texcoord.y;
      texcoords.push_back(texcoord);
    } else if (identifier == "vn") {
      Vector3 normal;
      s >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    } else if (identifier == "f") {
      VertexData triangle[3];

      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
        std::string vertexDefinition;
        s >> vertexDefinition;
        if (vertexDefinition.empty()) {
          continue;
        }

        std::istringstream v(vertexDefinition);
        uint32_t elementIndices[3] = {0, 0, 0};

        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(v, index, '/');

          if (index.empty()) {
            elementIndices[element] = 0;
          } else {
            elementIndices[element] = static_cast<uint32_t>(std::stoi(index));
          }
        }

        if (elementIndices[0] == 0 || elementIndices[1] == 0 ||
            elementIndices[2] == 0) {
          continue;
        }
        if (elementIndices[0] - 1 >= positions.size()) {
          continue;
        }
        if (elementIndices[1] - 1 >= texcoords.size()) {
          continue;
        }
        if (elementIndices[2] - 1 >= normals.size()) {
          continue;
        }

        Vector4 position = positions[elementIndices[0] - 1];
        Vector2 texcoord = texcoords[elementIndices[1] - 1];
        Vector3 normal = normals[elementIndices[2] - 1];

        position.x *= -1.0f;
        normal.x *= -1.0f;
        texcoord.y = 1.0f - texcoord.y;

        triangle[faceVertex] = {position, texcoord, normal};
      }

      modelData.vertices.push_back(triangle[2]);
      modelData.vertices.push_back(triangle[1]);
      modelData.vertices.push_back(triangle[0]);
    } else if (identifier == "mtllib") {
      std::string materialFilename;
      s >> materialFilename;
      if (!materialFilename.empty()) {
        modelData.material =
            LoadMaterialTemplateFile(directoryPath, materialFilename);
      }
    }
  }

  return modelData;
}

MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                      const std::string &filename) {
  MaterialData materialData;
  std::string line;

  const std::string fullPath = directoryPath + "/" + filename;
  std::ifstream file(fullPath);

  if (!file.is_open()) {
    LogFileOpenError_(fullPath);
    return materialData;
  }

  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier;

    if (identifier == "map_Kd") {
      std::string textureFilename;
      s >> textureFilename;
      if (!textureFilename.empty()) {
        materialData.textureFilePath = directoryPath + "/" + textureFilename;
      }
    }
  }

  return materialData;
}
