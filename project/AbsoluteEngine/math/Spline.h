#pragma once
#include "Vector.h"
#include <vector>

class Spline {
public:
    /// <summary>
    /// Catmull-Rom補間を用いて座標を計算する
    /// </summary>
    /// <param name="p0">制御点0</param>
    /// <param name="p1">制御点1（開始点）</param>
    /// <param name="p2">制御点2（終了点）</param>
    /// <param name="p3">制御点3</param>
    /// <param name="t">補間係数 (0.0 - 1.0)</param>
    /// <returns>補間された座標</returns>
    static Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

    /// <summary>
    /// ウェイポイントのリストから、全体の進捗率に応じた座標を計算する
    /// </summary>
    /// <param name="points">ウェイポイントのリスト</param>
    /// <param name="t">全体の進捗率 (0.0 - 1.0)</param>
    /// <returns>補間された座標</returns>
    static Vector3 GetPoint(const std::vector<Vector3>& points, float t);
};
