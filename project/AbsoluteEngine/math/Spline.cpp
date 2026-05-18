#include "Spline.h"
#include <cmath>
#include <algorithm>

Vector3 Spline::CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    Vector3 result;
    result.x = 0.5f * (
        (2.0f * p1.x) +
        (-p0.x + p2.x) * t +
        (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
        (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3
    );
    result.y = 0.5f * (
        (2.0f * p1.y) +
        (-p0.y + p2.y) * t +
        (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
        (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3
    );
    result.z = 0.5f * (
        (2.0f * p1.z) +
        (-p0.z + p2.z) * t +
        (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 +
        (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3
    );

    return result;
}

Vector3 Spline::GetPoint(const std::vector<Vector3>& points, float t) {
    if (points.empty()) return { 0, 0, 0 };
    if (points.size() == 1) return points[0];

    // t を 0.0 - 1.0 にクランプ
    t = std::clamp(t, 0.0f, 1.0f);

    // 区間の数を計算
    size_t numSections = points.size() - 1;
    float sectionProgress = t * (float)numSections;
    size_t index = (size_t)std::floor(sectionProgress);
    if (index >= numSections) index = numSections - 1;

    float localT = sectionProgress - (float)index;

    // 4つの制御点を決定（端点では隣接点を代用）
    const Vector3& p1 = points[index];
    const Vector3& p2 = points[index + 1];
    const Vector3& p0 = (index == 0) ? p1 : points[index - 1];
    const Vector3& p3 = (index + 2 >= points.size()) ? p2 : points[index + 2];

    return CatmullRom(p0, p1, p2, p3, localT);
}
