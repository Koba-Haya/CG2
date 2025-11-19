#pragma once

#include "Model.h" // ModelData, MaterialData
#include <string>

// OBJ ファイルを読み込んで ModelData を構築する
ModelData LoadObjFile(const std::string &directoryPath,
                      const std::string &filename);

// MTL ファイルを読み込んで MaterialData を構築する
MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                      const std::string &filename);