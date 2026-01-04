#include "GameCamera.h"
#include "Input.h"

void GameCamera::SetController(std::unique_ptr<ICameraController> controller, const CameraContext& ctx) {
    if (controller_) {
        controller_->OnExit(*this, ctx);
    }
    controller_ = std::move(controller);
    if (controller_) {
        controller_->OnEnter(*this, ctx);
    }
}

void GameCamera::Update(const Input& /*input*/) {
    if (controller_) {
        controller_->Update(*this, ctx_);
    }
    view_ = MakeLookAtMatrix(eye_, target_, up_);
}