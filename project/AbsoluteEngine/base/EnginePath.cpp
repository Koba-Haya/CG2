#include "EnginePath.h"
#include <iostream>

namespace AbsoluteEngine {

// 実際の値はGameApp::Initialize()がビルド時のプロジェクトパス(APPLICATION_ROOT_DIR)で
// SetApplicationRoot()を呼んで上書きする。この初期値はそれが呼ばれるまでのプレースホルダ
std::string EnginePath::applicationRoot_ = "Application/";
bool EnginePath::isRootSet_ = false;

void EnginePath::SetApplicationRoot(const std::string& path) {
    applicationRoot_ = path;
    isRootSet_ = true;
    // 末尾の/を保証する
    if (!applicationRoot_.empty() && applicationRoot_.back() != '/' && applicationRoot_.back() != '\\') {
        applicationRoot_ += "/";
    }
}

std::string EnginePath::GetApplicationRoot() {
    return applicationRoot_;
}

std::string EnginePath::Resolve(const std::string& relativePath) {
    if (!isRootSet_) {
        // SetApplicationRoot()より前にResolve()が呼ばれると、プレースホルダ値のまま
        // それらしく見えるが誤ったパスを返してしまう。原因究明しやすいよう一度だけ警告する
        static bool warned = false;
        if (!warned) {
            std::cerr << "[EnginePath] 警告: SetApplicationRoot()が呼ばれる前にResolve()が使用されました。"
                          "パスが意図した場所を指していない可能性があります。" << std::endl;
            warned = true;
        }
    }
    return applicationRoot_ + relativePath;
}

} // namespace AbsoluteEngine
