#pragma once
#include <string>

namespace AbsoluteEngine {

class EnginePath {
public:
    // アプリケーションのルートパスを設定する
    static void SetApplicationRoot(const std::string& path);

    // アプリケーションのルートパスを取得する
    static std::string GetApplicationRoot();

    // アプリケーションルートからの相対パスを絶対パス（または完全な相対パス）に解決する
    static std::string Resolve(const std::string& relativePath);

private:
    static std::string applicationRoot_;
};

} // namespace AbsoluteEngine
