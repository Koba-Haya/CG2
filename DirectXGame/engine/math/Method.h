#pragma once
#include "Matrix.h"
#include "Vector.h"
#include <assert.h>
#include <cmath>

/// <summary>
/// 単位行列作成関数
/// </summary>
/// <returns>作成された行列</returns>
Matrix4x4 MakeIdentity4x4();

/// <summary>
/// 3次元アフィン変換行列
/// </summary>
/// <param name="scale">拡縮率</param>
/// <param name="rotate">回転率</param>
/// <param name="translate">移動</param>
/// <returns>変換結果</returns>
Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                           const Vector3 &translate);

/// <summary>
/// 行列の積
/// </summary>
/// <param name="m1">計算される行列1</param>
/// <param name="m2">計算される行列2</param>
/// <returns>計算結果</returns>
Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2);

/// <summary>
/// X軸回転行列
/// </summary>
/// <param name="radian">ラジアン</param>
/// <returns>回転後行列</returns>
Matrix4x4 MakeRotateXMatrix(float radian);

/// <summary>
/// Y軸回転行列
/// </summary>
/// <param name="radian">ラジアン</param>
/// <returns>回転後行列</returns>
Matrix4x4 MakeRotateYMatrix(float radian);

/// <summary>
/// Z軸回転行列
/// </summary>
/// <param name="radian">ラジアン</param>
/// <returns>回転後行列</returns>
Matrix4x4 MakeRotateZMatrix(float radian);

/// <summary>
/// 平行移動行列
/// </summary>
/// <param name="translate">平行移動を行うパラメータ</param>
/// <returns>変換後の行列</returns>
Matrix4x4 MakeTranslateMatrix(const Vector3 &translate);

/// <summary>
/// 拡大縮小行列
/// </summary>
/// <param name="scale">拡縮率</param>
/// <returns>変換後行列</returns>
Matrix4x4 MakeScaleMatrix(const Vector3 &scale);

/// <summary>
/// 透視投影行列
/// </summary>
/// <param name="fovY">画角Y</param>
/// <param name="aspectRatio">アスペクト比</param>
/// <param name="nearClip">近平面への距離</param>
/// <param name="farClip">遠平面への距離</param>
/// <returns>切り取る範囲</returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip);

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip);

/// <summary>
/// 逆行列
/// </summary>
/// <param name="m">変換される行列</param>
/// <returns>変換結果</returns>
Matrix4x4 Inverse(const Matrix4x4 &m);

/// <summary>
/// 内積
/// </summary>
/// <param name="v1">計算されるベクトル１</param>
/// <param name="v2">計算されるベクトル２</param>
/// <returns>合計値</returns>
float Dot(const Vector3 &v1, const Vector3 &v2);

/// <summary>
/// 長さ（ノルム）
/// </summary>
/// <param name="v">ベクトル</param>
/// <returns>長さ</returns>
float Length(const Vector3 &v);

Vector3 Normalize(const Vector3 &v);

Vector3 Cross(const Vector3 &v1, const Vector3 &v2);

Matrix4x4 MakeLookAtMatrix(const Vector3 &eye, const Vector3 &target,
                           const Vector3 &up);

Vector3 TransformNormal(const Vector3 &v, const Matrix4x4 &m);

/// <summary>
/// 加算
/// </summary>
/// <param name="v1">加算するベクトル１</param>
/// <param name="v2">加算するベクトル２</param>
/// <returns>加算合計ベクトル</returns>
Vector3 Add(const Vector3 &v1, const Vector3 &v2);
