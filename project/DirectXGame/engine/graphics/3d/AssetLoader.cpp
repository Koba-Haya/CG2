#include "AssetLoader.h"
#include <algorithm>
#include <cassert>
#include <filesystem>

static std::string ToLower_(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return s;
}

static std::string GetExtLower_(const std::string &filename) {
  auto pos = filename.find_last_of('.');
  if (pos == std::string::npos)
    return {};
  return ToLower_(filename.substr(pos + 1));
}

std::shared_ptr<const ModelData>
AssetLoader::LoadModel(const std::string &directoryPath,
                       const std::string &filename) {
  const std::string key = MakeKey_(directoryPath, filename);
  if (auto it = cache_.find(key); it != cache_.end()) {
    return it->second;
  }

  Assimp::Importer importer;
  const std::string filePath = directoryPath + "/" + filename;

  const unsigned flags = aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                         aiProcess_GenSmoothNormals |
                         aiProcess_JoinIdenticalVertices;

  const aiScene *scene = importer.ReadFile(filePath.c_str(), flags);
  assert(scene);
  assert(scene->mRootNode);

  const std::string ext = GetExtLower_(filename);

  UVFixupOptions uvOpt{};
  if (ext == "obj") {
    uvOpt.flipV = false;
    uvOpt.flipU = false;
  } else if (ext == "gltf" || ext == "glb") {
    uvOpt.flipV = true;
    uvOpt.flipU = true;
  } else {
    uvOpt.flipV = false;
    uvOpt.flipU = false;
  }

  auto modelData = std::make_shared<ModelData>();

  // Materials
  modelData->materials.resize(scene->mNumMaterials);
  for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
    aiMaterial *mat = scene->mMaterials[i];
    MaterialData md{};

    if (mat && mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
      aiString tex;
      mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex);
      md.textureFilePath = directoryPath + "/" + tex.C_Str();
    }
    modelData->materials[i] = std::move(md);
  }

  // Meshes
  modelData->meshes.resize(scene->mNumMeshes);
  for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
    const aiMesh *mesh = scene->mMeshes[meshIndex];
    assert(mesh);

    MeshData meshData;
    meshData.materialIndex = (mesh->mMaterialIndex < scene->mNumMaterials)
                                 ? static_cast<int>(mesh->mMaterialIndex)
                                 : -1;

    for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
      const aiFace &face = mesh->mFaces[faceIndex];
      if (face.mNumIndices != 3) {
        continue;
      }

      VertexData tri[3]{};

      for (uint32_t e = 0; e < 3; ++e) {
        const uint32_t vi = face.mIndices[e];

        const aiVector3D &p = mesh->mVertices[vi];
        const aiVector3D &n =
            mesh->HasNormals() ? mesh->mNormals[vi] : aiVector3D(0, 1, 0);
        const aiVector3D &uv = (mesh->HasTextureCoords(0))
                                   ? mesh->mTextureCoords[0][vi]
                                   : aiVector3D(0, 0, 0);

        VertexData v{};
        v.position = {p.x, p.y, p.z, 1.0f};
        v.normal = {n.x, n.y, n.z};
        v.texcoord = {uv.x, uv.y};

        tri[e] = FixupVertex_AssimpToEngine(v, uvOpt);
      }

      meshData.vertices.push_back(tri[0]);
      meshData.vertices.push_back(tri[1]);
      meshData.vertices.push_back(tri[2]);
    }

    modelData->meshes[meshIndex] = std::move(meshData);
  }

  modelData->rootNode = ReadNode_(scene->mRootNode);

  cache_[key] = modelData;
  return modelData;
}

std::shared_ptr<const ModelData>
AssetLoader::LoadModel(const std::string &path) {
  std::filesystem::path p(path);
  const std::string dir = p.parent_path().string();
  const std::string file = p.filename().string();
  return LoadModel(dir, file);
}

Node AssetLoader::ReadNode_(const aiNode *node) {
  Node result{};

  result.localMatrix = ConvertAssimpMatrix(node->mTransformation);
  result.name = node->mName.C_Str();

  result.meshIndices.reserve(node->mNumMeshes);
  for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
    result.meshIndices.push_back(node->mMeshes[i]);
  }

  result.children.reserve(node->mNumChildren);
  for (uint32_t i = 0; i < node->mNumChildren; ++i) {
    result.children.push_back(ReadNode_(node->mChildren[i]));
  }

  return result;
}
