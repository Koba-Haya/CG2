#define NOMINMAX
#include "GameHUD.h"
#include <algorithm>

void GameHUD::Initialize() {
    // センターエイム初期化（ロックオン範囲を示すために大きくする。半径150なので直径300）
    Sprite::CreateInfo centerInfo;
    centerInfo.texturePath = "resources/app/particle/circle.png";
    centerInfo.size = {150.0f, 150.0f}; 
    centerInfo.color = {0.0f, 1.0f, 0.0f, 0.3f}; // 半透明の緑
    
    centerAimSprite_ = std::make_unique<Sprite>();
    centerAimSprite_->Initialize(centerInfo);
    
    // カーソル初期化（常時表示用）
    Sprite::CreateInfo cursorInfo;
    cursorInfo.texturePath = "resources/app/particle/circle.png";
    cursorInfo.size = {60.0f, 60.0f}; 
    cursorInfo.color = {1.0f, 1.0f, 1.0f, 0.9f}; // 白色
    
    cursorSprite_ = std::make_unique<Sprite>();
    cursorSprite_->Initialize(cursorInfo);
    
    // HPスプライトやロックオンスプライトの事前生成
    // 最大表示数分を確保しておく
    for (int i = 0; i < 10; ++i) {
        auto hpSp = std::make_unique<Sprite>();
        Sprite::CreateInfo hpInfo;
        hpInfo.texturePath = "resources/app/particle/circle.png";
        hpInfo.size = {32.0f, 32.0f};
        hpInfo.color = {0.0f, 1.0f, 0.0f, 1.0f}; // 緑色
        hpSp->Initialize(hpInfo);
        hpSprites_.push_back(std::move(hpSp));

        auto lockSp = std::make_unique<Sprite>();
        Sprite::CreateInfo lockInfo;
        lockInfo.texturePath = "resources/app/particle/circle.png";
        lockInfo.size = {48.0f, 48.0f};
        lockInfo.color = {1.0f, 0.0f, 0.0f, 0.8f}; // 赤色
        lockSp->Initialize(lockInfo);
        lockOnCursorSprites_.push_back(std::move(lockSp));
    }
}

void GameHUD::Update(int playerHp, int playerMaxHp, const std::vector<Vector2>& lockOnPositions, bool isLockingMode, const Vector2& cursorPos) {
    currentHp_ = playerHp;
    maxHp_ = playerMaxHp;
    isLockingMode_ = isLockingMode;
    currentLockOnPositions_ = lockOnPositions;

    // HPスプライトの更新
    // 画面左下 (x: 50 + i*40, y: 650) 付近に並べる
    for (int i = 0; i < hpSprites_.size(); ++i) {
        if (i < maxHp_) {
            // サイズが32なので、中心を合わせるなら少しずらす
            Vector3 pos = {50.0f + i * 40.0f - 16.0f, 650.0f - 16.0f, 0.0f};
            hpSprites_[i]->SetPosition(pos);
            if (i < currentHp_) {
                hpSprites_[i]->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); // 緑（残りHP）
            } else {
                hpSprites_[i]->SetColor({0.5f, 0.5f, 0.5f, 0.5f}); // 灰色（減ったHP）
            }
        }
    }

    // センターエイムとカーソルの更新
    if (isLockingMode_) {
        // センターエイムのサイズは150なので、左上座標は cursorPos - 75
        centerAimSprite_->SetPosition({cursorPos.x - 75.0f, cursorPos.y - 75.0f, 0.0f});
        cursorSprite_->SetColor({0.0f, 1.0f, 0.0f, 0.9f}); // ロック中は緑色
    } else {
        cursorSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.9f}); // 通常時は白色
    }
    
    // カーソルのサイズは60なので、左上座標は cursorPos - 30
    cursorSprite_->SetPosition({cursorPos.x - 30.0f, cursorPos.y - 30.0f, 0.0f});

    // ロックオンカーソルの更新
    for (int i = 0; i < lockOnCursorSprites_.size(); ++i) {
        if (i < currentLockOnPositions_.size()) {
            // ロックオンカーソルのサイズは48なので、左上座標は x-24, y-24
            Vector3 pos = {currentLockOnPositions_[i].x - 24.0f, currentLockOnPositions_[i].y - 24.0f, 0.0f};
            lockOnCursorSprites_[i]->SetPosition(pos);
        }
    }
}

void GameHUD::Draw() {
    // HP描画
    int drawHpCount = maxHp_ < (int)hpSprites_.size() ? maxHp_ : (int)hpSprites_.size();
    for (int i = 0; i < drawHpCount; ++i) {
        hpSprites_[i]->Draw();
    }

    // カーソルは常時描画
    if (cursorSprite_) {
        cursorSprite_->Draw();
    }

    // センターエイム描画（ロックオン時のみ）
    if (isLockingMode_) {
        centerAimSprite_->Draw();
    }

    // ロックオンカーソル描画
    size_t drawLockCount = currentLockOnPositions_.size() < lockOnCursorSprites_.size() ? currentLockOnPositions_.size() : lockOnCursorSprites_.size();
    for (size_t i = 0; i < drawLockCount; ++i) {
        lockOnCursorSprites_[i]->Draw();
    }
}
