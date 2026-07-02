#include "EnginePath.h"

namespace AbsoluteEngine {

std::string EnginePath::applicationRoot_ = "C:/Users/haya2/source/repos/CG2/project/Application/"; // 初期値は後方互換性のため一旦残すか、"Application/" にする

void EnginePath::SetApplicationRoot(const std::string& path) {
    applicationRoot_ = path;
    // 末尾の/を保証する
    if (!applicationRoot_.empty() && applicationRoot_.back() != '/' && applicationRoot_.back() != '\\') {
        applicationRoot_ += "/";
    }
}

std::string EnginePath::GetApplicationRoot() {
    return applicationRoot_;
}

std::string EnginePath::Resolve(const std::string& relativePath) {
    return applicationRoot_ + relativePath;
}

} // namespace AbsoluteEngine
