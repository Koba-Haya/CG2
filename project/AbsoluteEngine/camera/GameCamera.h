#pragma once
#include <memory>

#include "Camera.h"
#include "ICameraController.h"

class GameCamera final : public Camera {
public:
	void Initialize() override {}
	void Update(const Input& input) override;

	void SetController(std::unique_ptr<ICameraController> controller, const CameraContext& ctx);
	void SetContext(const CameraContext& ctx) { ctx_ = ctx; }
	const CameraContext& GetContext() const { return ctx_; }

	void SetEye(const Vector3& eye) { eye_ = eye; }
	void SetTarget(const Vector3& target) { target_ = target; }
	void SetUp(const Vector3& up) { up_ = up; }

	const Vector3& GetEye() const { return eye_; }
	const Vector3& GetTarget() const { return target_; }
	const Vector3& GetUp() const { return up_; }

private:
	CameraContext ctx_{};
	std::unique_ptr<ICameraController> controller_;

	Vector3 eye_{ 0.0f, 2.0f, -10.0f };
	Vector3 target_{ 0.0f, 0.0f, 0.0f };
	Vector3 up_{ 0.0f, 1.0f, 0.0f };
};