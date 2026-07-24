#pragma once
#include "Vector.h"
#include <vector>

class Spline {
public:
    /// <summary>
    /// 弧長(実距離)ベースで進行するためのルックアップテーブル
    /// t(0-1)を等間隔サンプリングして累積距離を保持し、距離→tの逆引きに使う
    /// ウェイポイントが変更されたら Build() で再構築する必要がある
    /// </summary>
    class ArcLengthTable {
    public:
        // サンプル数を指定してテーブルを構築する（多いほど精度が上がるが計算コストも増える）
        void Build(const std::vector<Vector3>& points, int sampleCount = 200);

        // テーブル全体の弧長（＝レール全体の実距離）を取得する
        float GetTotalLength() const { return cumulativeLengths_.empty() ? 0.0f : cumulativeLengths_.back(); }

        // 距離(0 - GetTotalLength())に対応する t(0-1) を線形補間で求める
        float GetTAtDistance(float distance) const;

    private:
        // cumulativeLengths_[i] は t = i / (sampleCount-1) までの累積距離
        std::vector<float> cumulativeLengths_;
    };

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
