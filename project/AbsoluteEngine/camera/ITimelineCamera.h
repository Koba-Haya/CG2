#pragma once
#include "../Type/Vector.h"

namespace AbsoluteEngine {

// タイムラインが「レールに沿って進行するカメラ」を操作するための最小インターフェース。
// 具体的なレール実装（ウェイポイント方式か等）はApplication層に委ねることで、
// エンジン(TimelineManager/BaseScene)がApplication層の具象クラスに依存しないようにする。
class ITimelineCamera {
public:
    virtual ~ITimelineCamera() = default;

    // レールを最初から最後まで等速で走破するのに必要な秒数を返す
    virtual float GetDuration() const = 0;

    // レール上の指定progress(0-1)における位置・前方向ベクトルを計算する（読み取り専用、カメラは動かさない）
    virtual void GetPointAndForward(float progress, Vector3& outPos, Vector3& outForward) const = 0;

    // タイムラインからカメラ進行度(0-1)を直接設定する
    virtual void SetProgress(float progress) = 0;
};

} // namespace AbsoluteEngine
