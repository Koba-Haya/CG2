#pragma once
#include <memory>
#include <vector>
#include "Type/Vector.h"

// Spriteクラスの前方宣言だとスマートポインタやvectorで不便なため、適宜ヘッダーをインクルードするか確認
// Engine側(AbsoluteEngine)のパスに合わせる
#include "graphics/2d/Sprite.h"

class GameHUD {
public:
    GameHUD() = default;
    ~GameHUD() = default;

    void Initialize();
    
    // プレイヤーのHPと、ロックオン対象のスクリーン座標を渡して表示状態を更新
    void Update(int currentHp, int maxHp, const std::vector<Vector2>& lockOnPositions, bool isLockingMode, const Vector2& cursorPos);
    
    // スプライトの描画
    void Draw();

private:
    // カーソル（常時表示用）
    std::unique_ptr<Sprite> cursorSprite_;
    
    // HP表示用のスプライト群
    std::vector<std::unique_ptr<Sprite>> hpSprites_;
    
    // ロックオンカーソル用スプライト群
    std::vector<std::unique_ptr<Sprite>> lockOnCursorSprites_;

    // 自機用の中央照準（長押し中に表示）
    std::unique_ptr<Sprite> centerAimSprite_;

    int currentHp_ = 0;
    int maxHp_ = 0;
    bool isLockingMode_ = false;
    std::vector<Vector2> currentLockOnPositions_;
};
