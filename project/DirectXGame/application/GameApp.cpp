#define NOMINMAX
#include "GameApp.h"
#include "GameCamera.h"
#include "DebugCamera.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "TextureResource.h"
#include "DirectXResourceUtils.h"
#include "ModelUtils.h"
#include "TextureUtils.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <objbase.h>
#include <random>

namespace {
	std::mt19937& GetRNG() {
		static std::mt19937 rng{ std::random_device{}() };
		return rng;
	}
	float Rand01() {
		static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		return dist(GetRNG());
	}
	float RandRange(float minV, float maxV) { return minV + (maxV - minV) * Rand01(); }

	Vector3 HSVtoRGB(const Vector3& hsv) {
		float h = hsv.x;
		float s = hsv.y;
		float v = hsv.z;

		if (s <= 0.0f) {
			return { v, v, v };
		}

		h = std::fmod(h, 1.0f);
		if (h < 0.0f) {
			h += 1.0f;
		}

		float hf = h * 6.0f;
		int i = static_cast<int>(std::floor(hf));
		float f = hf - static_cast<float>(i);

		float p = v * (1.0f - s);
		float q = v * (1.0f - s * f);
		float t = v * (1.0f - s * (1.0f - f));

		switch (i % 6) {
		case 0: return { v, t, p };
		case 1: return { q, v, p };
		case 2: return { p, v, t };
		case 3: return { p, q, v };
		case 4: return { t, p, v };
		default: return { v, p, q };
		}
	}
} // namespace

GameApp::GameApp()
	: winApp_(), dx_(), input_(), audio_(), transform_{}, cameraTransform_{},
	transformSprite_{}, uvTransformSprite_{}, transform2_{} {
}

GameApp::~GameApp() { Finalize(); }

bool GameApp::Initialize() {
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	InitLogging_();

	winApp_.Initialize();

	DirectXCommon::InitParams params{
		winApp_.GetHInstance(),
		winApp_.GetHwnd(),
		WinApp::kClientWidth,
		WinApp::kClientHeight,
	};
	dx_.Initialize(params);

	// SRV/Textureマネージャーの初期化
	TextureManager::GetInstance()->Initialize(&dx_);
	ModelManager::GetInstance()->Initialize(&dx_, &dx_.GetSrvAllocator());

	// ImGui初期化
	imgui_.Initialize(&winApp_, &dx_);

	bool inputOk = input_.Initialize(winApp_.GetHInstance(), winApp_.GetHwnd());
	assert(inputOk);

	bool audioOk = audio_.Initialize();
	assert(audioOk);

	shaderCompiler_.Initialize(dx_.GetDXCUtils(), dx_.GetDXCCompiler(), dx_.GetDXCIncludeHandler());

	InitPipelines_();
	InitResources_();

	// カメラ生成（今はデバッグカメラを Camera インターフェースで包んで使う）
	camera_ = std::make_unique<DebugCamera>();
	camera_->Initialize();
	InitCamera_();

	accelerationField_.acceleration = { 15.0f, 0.0f, 0.0f };
	accelerationField_.area.min = { -1.0f, -1.0f, -1.0f };
	accelerationField_.area.max = { 1.0f, 1.0f, 1.0f };

	return true;
}

int GameApp::Run() {
	if (!Initialize()) {
		return -1;
	}

	while (winApp_.ProcessMessage()) {
		input_.Update();
		Update();
		Draw();
	}

	Finalize();
	return 0;
}

void GameApp::Finalize() {
	static bool finalized = false;
	if (finalized) {
		return;
	}
	finalized = true;

	// リソース解放
	imgui_.Finalize();

	audio_.Shutdown();
	input_.Finalize();

	winApp_.Finalize();

	ParticleManager::GetInstance()->Finalize();
	CoUninitialize();
}

void GameApp::Update() {
	if (camera_) {
		camera_->Update(input_);
		view3D_ = camera_->GetViewMatrix();
		proj3D_ = camera_->GetProjectionMatrix();
	} else {
		view3D_ = Inverse(MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, -10.0f }));
		proj3D_ = MakePerspectiveFovMatrix(
			0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
	}

#ifdef USE_IMGUI
	imgui_.Begin();

	ImGui::Begin("Particles");

	auto& ep = particleEmitter_.GetParams();

	// ===== Emitter =====
	ImGui::SeparatorText("Emitter");

	ImGui::Checkbox("Show Emitter Gizmo", &showEmitterGizmo_);

	// 形状
	{
		int shapeIndex = (ep.shape == EmitterShape::Box) ? 0 : 1;
		const char* shapeItems[] = { "Box", "Sphere" };
		if (ImGui::Combo("Emitter Shape", &shapeIndex, shapeItems, 2)) {
			ep.shape = (shapeIndex == 0) ? EmitterShape::Box : EmitterShape::Sphere;
		}
	}

	// エミッタ範囲を min/max で編集（UI用に変換）
	Vector3 emitterMin = {
		ep.localCenter.x - ep.extent.x,
		ep.localCenter.y - ep.extent.y,
		ep.localCenter.z - ep.extent.z
	};
	Vector3 emitterMax = {
		ep.localCenter.x + ep.extent.x,
		ep.localCenter.y + ep.extent.y,
		ep.localCenter.z + ep.extent.z
	};

	ImGui::DragFloat3("Emitter Min", reinterpret_cast<float*>(&emitterMin), 0.01f);
	ImGui::DragFloat3("Emitter Max", reinterpret_cast<float*>(&emitterMax), 0.01f);

	// min/maxの整形
	emitterMin.x = std::min(emitterMin.x, emitterMax.x);
	emitterMin.y = std::min(emitterMin.y, emitterMax.y);
	emitterMin.z = std::min(emitterMin.z, emitterMax.z);

	emitterMax.x = std::max(emitterMin.x, emitterMax.x);
	emitterMax.y = std::max(emitterMin.y, emitterMax.y);
	emitterMax.z = std::max(emitterMin.z, emitterMax.z);

	// center/extentsへ戻す（これで “いじっても変わらない” が解消される）
	ep.localCenter = {
		(emitterMin.x + emitterMax.x) * 0.5f,
		(emitterMin.y + emitterMax.y) * 0.5f,
		(emitterMin.z + emitterMax.z) * 0.5f
	};
	ep.extent = {
		(emitterMax.x - emitterMin.x) * 0.5f,
		(emitterMax.y - emitterMin.y) * 0.5f,
		(emitterMax.z - emitterMin.z) * 0.5f
	};

	// 発生頻度
	ImGui::SliderFloat("Emit Rate (per sec)", &ep.emitRate, 0.0f, 500.0f);

	// 寿命
	ImGui::SliderFloat("Life Min", &ep.lifeMin, 0.01f, 20.0f);
	ImGui::SliderFloat("Life Max", &ep.lifeMax, 0.01f, 20.0f);
	if (ep.lifeMax < ep.lifeMin) {
		ep.lifeMax = ep.lifeMin;
	}

	// カラーモード
	{
		int mode = static_cast<int>(ep.colorMode);
		const char* items[] = { "RandomRGB", "RangeRGB", "RangeHSV", "Fixed" };
		ImGui::Combo("Color Mode", &mode, items, 4);
		ep.colorMode = static_cast<ParticleColorMode>(mode);
	}

	// 色（ベース）
	ImGui::ColorEdit4("Base Color", reinterpret_cast<float*>(&ep.baseColor));

	// RangeRGB パラメータ
	if (ep.colorMode == ParticleColorMode::RangeRGB) {
		ImGui::SliderFloat3("RGB Range (+/-)", reinterpret_cast<float*>(&ep.rgbRange), 0.0f, 1.0f);
	}

	// RangeHSV パラメータ
	if (ep.colorMode == ParticleColorMode::RangeHSV) {
		ImGui::SliderFloat3("Base HSV", reinterpret_cast<float*>(&ep.baseHSV), 0.0f, 1.0f);
		ImGui::SliderFloat3("HSV Range (+/-)", reinterpret_cast<float*>(&ep.hsvRange), 0.0f, 1.0f);
	}

	// 変更が見えない時のための “全消し”
	if (ImGui::Button("Reset Particles")) {
		ParticleManager::GetInstance()->ClearParticleGroup(ep.groupName);
	}

	// ===== Manager =====
	ImGui::SeparatorText("Manager");

	// 最大発生数（グループの上限）
	static int maxInstances = 1000;
	ImGui::SliderInt("Max Particles", &maxInstances, 0, static_cast<int>(kParticleCount_));
	maxInstances = std::clamp(maxInstances, 0, static_cast<int>(kParticleCount_));
	ParticleManager::GetInstance()->SetGroupInstanceLimit(ep.groupName, static_cast<uint32_t>(maxInstances));

	// ブレンドモード（Drawで使う particleBlendMode_ をUIで切替）
	{
		const char* blendItems[] = { "Alpha", "Add", "Subtract", "Multiply", "Screen" };
		ImGui::Combo("Blend Mode", &particleBlendMode_, blendItems, 5);
		particleBlendMode_ = std::clamp(particleBlendMode_, 0, 4);
	}

	// 加速度場
	ImGui::Checkbox("Enable Acceleration Field", &enableAccelerationField_);
	ImGui::DragFloat3("Accel", reinterpret_cast<float*>(&accelerationField_.acceleration), 0.1f, -100.0f, 100.0f);

	// 加速度場 範囲（最小最大生成範囲…ではなく “場の適用範囲”）
	ImGui::DragFloat3("Field AABB Min", reinterpret_cast<float*>(&accelerationField_.area.min), 0.1f);
	ImGui::DragFloat3("Field AABB Max", reinterpret_cast<float*>(&accelerationField_.area.max), 0.1f);

	// min/maxが逆転してたら直す
	accelerationField_.area.min.x = std::min(accelerationField_.area.min.x, accelerationField_.area.max.x);
	accelerationField_.area.min.y = std::min(accelerationField_.area.min.y, accelerationField_.area.max.y);
	accelerationField_.area.min.z = std::min(accelerationField_.area.min.z, accelerationField_.area.max.z);

	accelerationField_.area.max.x = std::max(accelerationField_.area.min.x, accelerationField_.area.max.x);
	accelerationField_.area.max.y = std::max(accelerationField_.area.min.y, accelerationField_.area.max.y);
	accelerationField_.area.max.z = std::max(accelerationField_.area.min.z, accelerationField_.area.max.z);

	ImGui::End();

	imgui_.End();
#endif

	// ===== パーティクル更新（Emitter → Manager）=====
	const float deltaTime = 1.0f / 60.0f;

	// 親位置に追従させたいなら第2引数で渡す（今は particleSpawnCenter_ を親とみなす）
	particleEmitter_.Update(deltaTime);

	ParticleManager::GetInstance()->SetEnableAccelerationField(enableAccelerationField_);
	ParticleManager::GetInstance()->SetAccelerationField(accelerationField_);

	// ここで StructuredBuffer(t1) に書き込まれて activeInstanceCount が作られる
	ParticleManager::GetInstance()->Update(view3D_, proj3D_, deltaTime);
}

void GameApp::Draw() {
	dx_.BeginFrame();
	auto* cmdList = dx_.GetCommandList();

	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		dx_.GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

	UINT backBufferIndex = dx_.GetSwapChain()->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx_.GetRTVHandle(backBufferIndex);

	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdList->RSSetViewports(1, &dx_.GetViewport());
	cmdList->RSSetScissorRects(1, &dx_.GetScissorRect());

	Matrix4x4 viewMatrix = view3D_;
	Matrix4x4 projectionMatrix = proj3D_;

	// ===== 3Dモデル描画 =====
	{
		Matrix4x4 worldSphere = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
		model_.SetWorldTransform(worldSphere);
		model_.SetLightingMode(lightingMode_);

		Matrix4x4 worldPlane = MakeAffineMatrix(transform2_.scale, transform2_.rotate, transform2_.translate);
		planeModel_.SetWorldTransform(worldPlane);

		cmdList->SetGraphicsRootSignature(objPipeline_.GetRootSignature());
		cmdList->SetPipelineState(objPipeline_.GetPipelineState());

		model_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
		planeModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
	}

	// ===== エミッタ範囲ギズモ描画 =====
	{
		if (showEmitterGizmo_) {
			const auto& ep = particleEmitter_.GetParams();

			if (ep.shape == EmitterShape::Box) {
				Vector3 scale = { ep.extent.x * 2.0f, ep.extent.y * 2.0f, ep.extent.z * 2.0f };
				Matrix4x4 world = MakeAffineMatrix(scale, { 0.0f, 0.0f, 0.0f }, ep.localCenter);
				emitterBoxModel_.SetWorldTransform(world);
				emitterBoxModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
			} else {
				// sphere は extent を半径扱い（楕円でもOK）
				Vector3 scale = { std::max(ep.extent.x, 0.001f), std::max(ep.extent.y, 0.001f), std::max(ep.extent.z, 0.001f) };
				Matrix4x4 world = MakeAffineMatrix(scale, { 0.0f, 0.0f, 0.0f }, ep.localCenter);
				emitterSphereModel_.SetWorldTransform(world);
				emitterSphereModel_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
			}
		}
	}

	// ===== パーティクル描画（ParticleManager）=====
	{
		UnifiedPipeline* currentParticlePipeline = &particlePipelineAlpha_;
		switch (particleBlendMode_) {
		case 1: currentParticlePipeline = &particlePipelineAdd_; break;
		case 2: currentParticlePipeline = &particlePipelineSub_; break;
		case 3: currentParticlePipeline = &particlePipelineMul_; break;
		case 4: currentParticlePipeline = &particlePipelineScreen_; break;
		default: break;
		}

		ParticleManager::GetInstance()->Draw(cmdList, currentParticlePipeline);
	}

#ifdef USE_IMGUI
	imgui_.Draw(cmdList);
#endif

	dx_.EndFrame();
}

void GameApp::InitLogging_() {
	std::filesystem::create_directory("logs");

	auto now = std::chrono::system_clock::now();
	auto nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	std::string logFilePath = std::string("logs/") + dateString + ".log";
	logStream_.open(logFilePath);
}

void GameApp::InitPipelines_() {
	auto* device = dx_.GetDevice();
	auto* dxcUtils = dx_.GetDXCUtils();
	auto* dxcCompiler = dx_.GetDXCCompiler();
	auto* includeHandler = dx_.GetDXCIncludeHandler();

	PipelineDesc objBase = UnifiedPipeline::MakeObject3DDesc();
	assert(objPipeline_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, objBase));

	{
		auto wire = objBase;
		wire.alphaBlend = false;
		wire.enableDepth = true;
		wire.cullMode = D3D12_CULL_MODE_NONE;
		wire.fillMode = D3D12_FILL_MODE_WIREFRAME;
		assert(emitterGizmoPipelineWire_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, wire));
	}

	PipelineDesc sprBase = UnifiedPipeline::MakeSpriteDesc();

	auto sprAlpha = sprBase;
	sprAlpha.blendMode = BlendMode::Alpha;
	assert(spritePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprAlpha));

	auto sprAdd = sprBase;
	sprAdd.blendMode = BlendMode::Add;
	assert(spritePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprAdd));

	auto sprSub = sprBase;
	sprSub.blendMode = BlendMode::Subtract;
	assert(spritePipelineSub_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprSub));

	auto sprMul = sprBase;
	sprMul.blendMode = BlendMode::Multiply;
	assert(spritePipelineMul_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprMul));

	auto sprScr = sprBase;
	sprScr.blendMode = BlendMode::Screen;
	assert(spritePipelineScreen_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, sprScr));

	PipelineDesc partBase = UnifiedPipeline::MakeParticleDesc();

	auto pAlpha = partBase;
	pAlpha.blendMode = BlendMode::Alpha;
	assert(particlePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pAlpha));

	auto pAdd = partBase;
	pAdd.blendMode = BlendMode::Add;
	assert(particlePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pAdd));

	auto pSub = partBase;
	pSub.blendMode = BlendMode::Subtract;
	assert(particlePipelineSub_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pSub));

	auto pMul = partBase;
	pMul.blendMode = BlendMode::Multiply;
	assert(particlePipelineMul_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pMul));

	auto pScr = partBase;
	pScr.blendMode = BlendMode::Screen;
	assert(particlePipelineScreen_.Initialize(device, dxcUtils, dxcCompiler, includeHandler, pScr));
}

void GameApp::InitResources_() {
	ComPtr<ID3D12Device> device;
	dx_.GetDevice()->QueryInterface(IID_PPV_ARGS(&device));

	// ===== Model =====
	{
		Model::CreateInfo ci{};
		ci.dx = &dx_;
		ci.pipeline = &objPipeline_;
		ci.modelData = LoadObjFile("resources/sphere", "sphere.obj");
		ci.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		ci.lightingMode = 1;
		bool okModel = model_.Initialize(ci);
		assert(okModel);
	}
	{
		Model::CreateInfo ci{};
		ci.dx = &dx_;
		ci.pipeline = &objPipeline_;
		ci.modelData = LoadObjFile("resources/plane", "plane.obj");
		ci.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		ci.lightingMode = 1;
		bool okPlane = planeModel_.Initialize(ci);
		assert(okPlane);
	}
	{
		Model::CreateInfo ci{};
		ci.dx = &dx_;
		ci.pipeline = &emitterGizmoPipelineWire_;
		ci.modelData = LoadObjFile("resources/sphere", "sphere.obj");
		ci.baseColor = { 0.3f, 0.8f, 1.0f, 0.3f };
		ci.lightingMode = 0;
		bool ok = emitterSphereModel_.Initialize(ci);
		assert(ok);
	}
	{
		Model::CreateInfo ci{};
		ci.dx = &dx_;
		ci.pipeline = &emitterGizmoPipelineWire_;
		ci.modelData = LoadObjFile("resources/cube", "cube.obj");
		ci.baseColor = { 1.0f, 0.8f, 0.2f, 0.3f };
		ci.lightingMode = 0;
		bool ok = emitterBoxModel_.Initialize(ci);
		assert(ok);
	}

	// ===== Sprite =====
	{
		Sprite::CreateInfo sprInfo{};
		sprInfo.dx = &dx_;
		sprInfo.pipeline = &spritePipelineAlpha_;
		sprInfo.texturePath = "resources/uvChecker.png";
		sprInfo.size = { 640.0f, 360.0f };
		sprInfo.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool okSprite = sprite_.Initialize(sprInfo);
		assert(okSprite);
	}

	// ===== Particle System (Manager + Emitter) =====
	{
		ParticleManager::GetInstance()->Initialize(&dx_);
		const bool ok = ParticleManager::GetInstance()->CreateParticleGroup(
			particleGroupName_, "resources/particle/circle.png", kParticleCount_);
		assert(ok);

		ParticleEmitter::Params params{};
		params.groupName = particleGroupName_;
		params.shape = EmitterShape::Box;
		params.localCenter = { 0.0f, 0.0f, 0.0f };
		params.extent = { 1.0f, 1.0f, 1.0f };
		params.baseDir = { 0.0f, 1.0f, 0.0f };
		params.dirRandomness = 0.5f;
		params.speedMin = 0.5f;
		params.speedMax = 2.0f;
		params.lifeMin = 1.0f;
		params.lifeMax = 3.0f;
		params.particleScale = { 0.5f, 0.5f, 0.5f };
		params.emitRate = 10.0f;
		params.colorMode = ParticleColorMode::RandomRGB;
		params.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };

		particleEmitter_.Initialize(ParticleManager::GetInstance(), params);

		particleEmitter_.Burst(std::min(initialParticleCount_, kParticleCount_));

		/*std::strncpy(particleGroupNameBuf_, params.groupName.c_str(), sizeof(particleGroupNameBuf_) - 1);
		particleGroupNameBuf_[sizeof(particleGroupNameBuf_) - 1] = '\0';*/
	}

	// ===== DirectionalLight CB =====
	{
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = sizeof(DirectionalLight);
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		HRESULT hr = device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&directionalLightCB_));
		assert(SUCCEEDED(hr));

		directionalLightCB_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
		directionalLightData_->color = { 1, 1, 1, 1 };
		directionalLightData_->direction = { 0, -1, 0 };
		directionalLightData_->intensity = 1.0f;
	}

	bool select = audio_.Load("select", L"resources/sound/select.mp3", 1.0f);
	assert(select);
	selectVol_ = 1.0f;

	struct Vtx { float px, py, pz; float u, v; };
	Vtx quad[4] = {
		{-0.5f,  0.5f, 0.0f, 0.0f, 0.0f},
		{ 0.5f,  0.5f, 0.0f, 1.0f, 0.0f},
		{-0.5f, -0.5f, 0.0f, 0.0f, 1.0f},
		{ 0.5f, -0.5f, 0.0f, 1.0f, 1.0f},
	};
	uint16_t idx[6] = { 0, 1, 2, 2, 1, 3 };

}

void GameApp::InitCamera_() {
	transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	cameraTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f} };
	transformSprite_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	uvTransformSprite_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	transform2_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f} };

	// 3D投影はカメラ側に持たせる（Update では view だけ更新する）
	if (camera_) {
		camera_->SetPerspective(
			0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
	}
}
