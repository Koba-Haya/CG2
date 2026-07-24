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

void Spline::ArcLengthTable::Build(const std::vector<Vector3>& points, int sampleCount) {
    cumulativeLengths_.clear();
    if (points.size() < 2 || sampleCount < 2) {
        cumulativeLengths_.push_back(0.0f);
        return;
    }

    cumulativeLengths_.reserve(sampleCount);
    cumulativeLengths_.push_back(0.0f);

    Vector3 prevPoint = Spline::GetPoint(points, 0.0f);
    for (int i = 1; i < sampleCount; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
        Vector3 currPoint = Spline::GetPoint(points, t);

        Vector3 diff = { currPoint.x - prevPoint.x, currPoint.y - prevPoint.y, currPoint.z - prevPoint.z };
        float segmentLength = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

        cumulativeLengths_.push_back(cumulativeLengths_.back() + segmentLength);
        prevPoint = currPoint;
    }
}

float Spline::ArcLengthTable::GetTAtDistance(float distance) const {
    const size_t sampleCount = cumulativeLengths_.size();
    if (sampleCount < 2) return 0.0f;

    const float totalLength = cumulativeLengths_.back();
    distance = std::clamp(distance, 0.0f, totalLength);

    // 距離が収まる区間を線形探索する（サンプル数は数百程度なので十分高速）
    for (size_t i = 1; i < sampleCount; ++i) {
        if (distance <= cumulativeLengths_[i]) {
            const float segStart = cumulativeLengths_[i - 1];
            const float segEnd = cumulativeLengths_[i];
            const float segLength = segEnd - segStart;
            const float localRatio = (segLength > 0.0f) ? (distance - segStart) / segLength : 0.0f;

            const float tStart = static_cast<float>(i - 1) / static_cast<float>(sampleCount - 1);
            const float tEnd = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
            return tStart + (tEnd - tStart) * localRatio;
        }
    }
    return 1.0f;
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
