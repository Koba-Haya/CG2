#define NOMINMAX

#include "Method.h"
#include <algorithm>

static float Clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

Matrix4x4 MakeIdentity4x4() {
  Matrix4x4 result;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        result.m[i][j] = 1;
      } else {
        result.m[i][j] = 0;
      }
    }
  }
  return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                           const Vector3 &translate) {
  Matrix4x4 rotateMatrix = Multiply(
      MakeRotateXMatrix(rotate.x),
      Multiply(MakeRotateYMatrix(rotate.y), MakeRotateZMatrix(rotate.z)));

  Matrix4x4 result;
  result = {scale.x * rotateMatrix.m[0][0],
            scale.x * rotateMatrix.m[0][1],
            scale.x * rotateMatrix.m[0][2],
            0,
            scale.y * rotateMatrix.m[1][0],
            scale.y * rotateMatrix.m[1][1],
            scale.y * rotateMatrix.m[1][2],
            0,
            scale.z * rotateMatrix.m[2][0],
            scale.z * rotateMatrix.m[2][1],
            scale.z * rotateMatrix.m[2][2],
            0,
            translate.x,
            translate.y,
            translate.z,
            1};
  return result;
}

Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2) {
  Matrix4x4 result;
  result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] +
                   m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0];
  result.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] +
                   m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1];
  result.m[0][2] = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] +
                   m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2];
  result.m[0][3] = m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] +
                   m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3];
  result.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] +
                   m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0];
  result.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] +
                   m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1];
  result.m[1][2] = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] +
                   m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2];
  result.m[1][3] = m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] +
                   m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3];
  result.m[2][0] = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] +
                   m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0];
  result.m[2][1] = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] +
                   m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1];
  result.m[2][2] = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] +
                   m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2];
  result.m[2][3] = m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] +
                   m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3];
  result.m[3][0] = m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] +
                   m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0];
  result.m[3][1] = m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] +
                   m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1];
  result.m[3][2] = m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] +
                   m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2];
  result.m[3][3] = m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] +
                   m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3];
  return result;
}

Matrix4x4 MakeRotateXMatrix(float radian) {
  Matrix4x4 result;
  result = {1,
            0,
            0,
            0,
            0,
            std::cosf(radian),
            std::sinf(radian),
            0,
            0,
            -std::sinf(radian),
            std::cosf(radian),
            0,
            0,
            0,
            0,
            1};

  return result;
}

Matrix4x4 MakeRotateYMatrix(float radian) {
  Matrix4x4 result;
  result = {std::cosf(radian), 0, -std::sinf(radian), 0, 0, 1, 0, 0,
            std::sinf(radian), 0, std::cosf(radian),  0, 0, 0, 0, 1};

  return result;
}

Matrix4x4 MakeRotateZMatrix(float radian) {
  Matrix4x4 result;
  result = {std::cosf(radian),
            std::sinf(radian),
            0,
            0,
            -std::sinf(radian),
            std::cosf(radian),
            0,
            0,
            0,
            0,
            1,
            0,
            0,
            0,
            0,
            1};

  return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3 &translate) {
  Matrix4x4 result;
  result = {1, 0, 0, 0, 0,           1,           0,           0,
            0, 0, 1, 0, translate.x, translate.y, translate.z, 1};
  return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3 &scale) {
  Matrix4x4 result;
  result = {scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0, 0, 1};
  return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {
  Matrix4x4 result;
  result = {(1.0f / aspectRatio) * (1.0f / std::tanf(fovY / 2.0f)),
            0,
            0,
            0,
            0,
            (1.0f / std::tanf(fovY / 2.0f)),
            0,
            0,
            0,
            0,
            farClip / (farClip - nearClip),
            1,
            0,
            0,
            (-nearClip * farClip) / (farClip - nearClip),
            0};
  return result;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip) {
  Matrix4x4 result;
  result = {2.0f / (right - left),
            0,
            0,
            0,
            0,
            2.0f / (top - bottom),
            0,
            0,
            0,
            0,
            1.0f / (farClip - nearClip),
            0,
            (left + right) / (left - right),
            (top + bottom) / (bottom - top),
            nearClip / (nearClip - farClip),
            1};
  return result;
}

Matrix4x4 Inverse(const Matrix4x4 &m) {
  float determinant;
  determinant = m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3] +
                m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] +
                m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2] -
                m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1] -
                m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3] -
                m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2] -
                m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3] -
                m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1] -
                m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2] +
                m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1] +
                m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3] +
                m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2] +
                m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3] +
                m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1] +
                m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2] -
                m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1] -
                m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3] -
                m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2] -
                m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0] -
                m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0] -
                m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0] +
                m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0] +
                m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0] +
                m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0];
  Matrix4x4 result;
  result = {
      (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] +
       m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][3] * m.m[2][2] * m.m[3][1] -
       m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2]) /
          determinant,
      (-m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] -
       m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[2][2] * m.m[3][1] +
       m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2]) /
          determinant,
      (m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] +
       m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[3][1] -
       m.m[0][2] * m.m[1][1] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2]) /
          determinant,
      (-m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] -
       m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][3] * m.m[1][2] * m.m[2][1] +
       m.m[0][2] * m.m[1][1] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2]) /
          determinant,
      (-m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] -
       m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][3] * m.m[2][2] * m.m[3][0] +
       m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2]) /
          determinant,
      (m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] +
       m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[2][2] * m.m[3][0] -
       m.m[0][2] * m.m[2][0] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2]) /
          determinant,
      (-m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] -
       m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][3] * m.m[1][2] * m.m[3][0] +
       m.m[0][2] * m.m[1][0] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2]) /
          determinant,
      (m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] +
       m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] -
       m.m[0][2] * m.m[1][0] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2]) /
          determinant,
      (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] +
       m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][3] * m.m[2][1] * m.m[3][0] -
       m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1]) /
          determinant,
      (-m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] -
       m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[2][1] * m.m[3][0] +
       m.m[0][1] * m.m[2][0] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1]) /
          determinant,
      (m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] +
       m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][3] * m.m[1][1] * m.m[3][0] -
       m.m[0][1] * m.m[1][0] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1]) /
          determinant,
      (-m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] -
       m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] +
       m.m[0][1] * m.m[1][0] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1]) /
          determinant,
      (-m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] -
       m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][2] * m.m[2][1] * m.m[3][0] +
       m.m[1][1] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1]) /
          determinant,
      (m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] +
       m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][2] * m.m[2][1] * m.m[3][0] -
       m.m[0][1] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1]) /
          determinant,
      (-m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] -
       m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][2] * m.m[1][1] * m.m[3][0] +
       m.m[0][1] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1]) /
          determinant,
      (m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] +
       m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][2] * m.m[1][1] * m.m[2][0] -
       m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1]) /
          determinant};
  return result;
}

Vector3 Cross(const Vector3 &v1, const Vector3 &v2) {
  return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
          v1.x * v2.y - v1.y * v2.x};
}

float Length(const Vector3 &v) {
  float result;
  result = {sqrtf(powf(v.x, 2) + powf(v.y, 2) + powf(v.z, 2))};
  return result;
}

Vector3 Normalize(const Vector3 &v) {
  float len = Length(v);
  return {v.x / len, v.y / len, v.z / len};
}

float Dot(const Vector3 &v1, const Vector3 &v2) {
  float result;
  result = (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
  return result;
}

Matrix4x4 MakeLookAtMatrix(const Vector3 &eye, const Vector3 &target,
                           const Vector3 &up) {
  Vector3 flont = {target.x - eye.x, target.y - eye.y, target.z - eye.z};
  Vector3 zAxis = Normalize(flont);            // カメラの前方
  Vector3 xAxis = Normalize(Cross(up, zAxis)); // カメラの右方向
  Vector3 yAxis = Cross(zAxis, xAxis);         // カメラの上方向

  Matrix4x4 result;

  result.m[0][0] = xAxis.x;
  result.m[0][1] = yAxis.x;
  result.m[0][2] = zAxis.x;
  result.m[0][3] = 0.0f;

  result.m[1][0] = xAxis.y;
  result.m[1][1] = yAxis.y;
  result.m[1][2] = zAxis.y;
  result.m[1][3] = 0.0f;

  result.m[2][0] = xAxis.z;
  result.m[2][1] = yAxis.z;
  result.m[2][2] = zAxis.z;
  result.m[2][3] = 0.0f;

  result.m[3][0] = -Dot(xAxis, eye);
  result.m[3][1] = -Dot(yAxis, eye);
  result.m[3][2] = -Dot(zAxis, eye);
  result.m[3][3] = 1.0f;

  return result;
}

Vector3 TransformNormal(const Vector3 &v, const Matrix4x4 &m) {
  return {
      v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
      v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
      v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2],
  };
}

Vector3 TransformPoint(const Vector3 &v, const Matrix4x4 &m) {
  Vector3 result;
  result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
  result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
  result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
  float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
  result.x /= w;
  result.y /= w;
  result.z /= w;
  return result;
}

Vector3 Add(const Vector3 &v1, const Vector3 &v2) {
  Vector3 result;
  result.x = v1.x + v2.x;
  result.y = v1.y + v2.y;
  result.z = v1.z + v2.z;
  return result;
}

Matrix4x4 Transpose(const Matrix4x4 &m) {
  Matrix4x4 r{};
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      r.m[y][x] = m.m[x][y];
    }
  }
  return r;
}

Vector3 Lerp(const Vector3 &v1, const Vector3 &v2, float t) {
  return {
    v1.x + (v2.x - v1.x) * t,
    v1.y + (v2.y - v1.y) * t,
    v1.z + (v2.z - v1.z) * t
  };
}

Quaternion IdentityQuaternion() {
  return {0.0f, 0.0f, 0.0f, 1.0f};
}

Quaternion Slerp(const Quaternion &q1, const Quaternion &q2, float t) {
  float cosOmega = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
  Quaternion q2_adjusted = q2;
  if (cosOmega < 0.0f) {
    q2_adjusted.x = -q2.x;
    q2_adjusted.y = -q2.y;
    q2_adjusted.z = -q2.z;
    q2_adjusted.w = -q2.w;
    cosOmega = -cosOmega;
  }
  
  float k0, k1;
  if (cosOmega > 0.9999f) {
    k0 = 1.0f - t;
    k1 = t;
  } else {
    float sinOmega = std::sqrt(1.0f - cosOmega * cosOmega);
    float omega = std::atan2(sinOmega, cosOmega);
    float invSinOmega = 1.0f / sinOmega;
    k0 = std::sin((1.0f - t) * omega) * invSinOmega;
    k1 = std::sin(t * omega) * invSinOmega;
  }
  
  Quaternion result;
  result.x = q1.x * k0 + q2_adjusted.x * k1;
  result.y = q1.y * k0 + q2_adjusted.y * k1;
  result.z = q1.z * k0 + q2_adjusted.z * k1;
  result.w = q1.w * k0 + q2_adjusted.w * k1;
  return result;
}

Matrix4x4 MakeRotateMatrix(const Quaternion &q) {
  Matrix4x4 result = MakeIdentity4x4();
  float xx = q.x * q.x;
  float yy = q.y * q.y;
  float zz = q.z * q.z;
  float xy = q.x * q.y;
  float xz = q.x * q.z;
  float yz = q.y * q.z;
  float wx = q.w * q.x;
  float wy = q.w * q.y;
  float wz = q.w * q.z;

  result.m[0][0] = 1.0f - 2.0f * (yy + zz);
  result.m[0][1] = 2.0f * (xy + wz);
  result.m[0][2] = 2.0f * (xz - wy);

  result.m[1][0] = 2.0f * (xy - wz);
  result.m[1][1] = 1.0f - 2.0f * (xx + zz);
  result.m[1][2] = 2.0f * (yz + wx);

  result.m[2][0] = 2.0f * (xz + wy);
  result.m[2][1] = 2.0f * (yz - wx);
  result.m[2][2] = 1.0f - 2.0f * (xx + yy);

  return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Quaternion &rotate, const Vector3 &translate) {
  Matrix4x4 rotateMatrix = MakeRotateMatrix(rotate);
  Matrix4x4 result;
  result = {scale.x * rotateMatrix.m[0][0],
            scale.x * rotateMatrix.m[0][1],
            scale.x * rotateMatrix.m[0][2],
            0,
            scale.y * rotateMatrix.m[1][0],
            scale.y * rotateMatrix.m[1][1],
            scale.y * rotateMatrix.m[1][2],
            0,
            scale.z * rotateMatrix.m[2][0],
            scale.z * rotateMatrix.m[2][1],
            scale.z * rotateMatrix.m[2][2],
            0,
            translate.x,
            translate.y,
            translate.z,
            1};
  return result;
}
