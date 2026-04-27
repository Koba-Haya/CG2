#pragma once

enum class BlendMode {
  Opaque = 0, // ブレンドなし
  Alpha,      // 通常のアルファブレンド
  Add,        // 加算
  Subtract,   // 減算
  Multiply,   // 乗算
  Screen,     // スクリーン
};
