#pragma once
#include "Matrix.h"
#include "Method.h"
#include "Vector.h"

 struct TransformationMatrix {
   Matrix4x4 WVP;
   Matrix4x4 World;
 };

 struct DirectionalLight {
   Vector4 color;     // ライトの色
   Vector3 direction; // ライトの向き
   float intensity;   // 輝度
 };

 struct Material {
   Vector4 color;
   int32_t enableLighting;
   float padding[3];
   Matrix4x4 uvTransform;
 };
