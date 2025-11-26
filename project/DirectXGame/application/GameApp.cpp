#define NOMINMAX
#include "GameApp.h"
#include "DebugCamera.h"
#include "DirectXResourceUtils.h"
#include "ModelUtils.h"
#include "TextureUtils.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <format>
#include <objbase.h> // CoInitializeEx, CoUninitialize
#include <random>

// ファイル先頭の方（名前空間外）に追加
namespace {
	std::mt19937& GetRNG() {
		static std::mt19937 rng{ std::random_device{}() };
		return rng;
	}
	float Rand01() {
		static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		return dist(GetRNG());
	}
	float RandRange(float minV, float maxV) {
		return minV + (maxV - minV) * Rand01();
	}
} // namespace

GameApp::GameApp()
	: winApp_(), dx_(), input_(), audio_(), transform_{}, cameraTransform_{},
	transformSprite_{}, uvTransformSprite_{}, transform2_{}, particles_{} {
}

GameApp::~GameApp() { Finalize(); }

bool GameApp::Initialize() {
	// COM 初期化
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	// ログディレクトリ & ログファイル
	InitLogging_();

	// WinApp 初期化
	winApp_.Initialize();

	// DirectX 初期化（InitParams に合わせる）
	DirectXCommon::InitParams params{
		winApp_.GetHInstance(),
		winApp_.GetHwnd(),
		WinApp::kClientWidth,
		WinApp::kClientHeight,
	};
	dx_.Initialize(params);

	// Input 初期化
	bool inputOk = input_.Initialize(winApp_.GetHInstance(), winApp_.GetHwnd());
	assert(inputOk);

	// Audio 初期化
	bool audioOk = audio_.Initialize();
	assert(audioOk);

	// SRV 割り当てヘルパ初期化（DirectXCommon の SRV ヒープを利用）
	srvAlloc_.Init(dx_.GetDevice(), dx_.GetSRVHeap());

	// ShaderCompiler 初期化（DirectXCommon の DXC を流用）
	shaderCompiler_.Initialize(dx_.GetDXCUtils(), dx_.GetDXCCompiler(),
		dx_.GetDXCIncludeHandler());

	// パイプライン初期化（Step3 のメイン）
	InitPipelines_();

	// リソース（モデル・スプライト・パーティクル等）初期化
	InitResources_();

	// カメラ関連
	InitCamera_();

	return true;
}

int GameApp::Run() {
	if (!Initialize()) {
		return -1;
	}

	// ウィンドウの×ボタンが押されるまでループ
	while (winApp_.ProcessMessage()) {
		// 入力更新
		input_.Update();

		// ゲーム更新
		Update();

		// 描画
		Draw();
	}

	Finalize();
	return 0;
}

void GameApp::Finalize() {
	static bool finalized = false;
	if (finalized)
		return;
	finalized = true;

	// TODO: DebugCamera delete, Resource 解放, ImGui 終了などを整理してここへ
	audio_.Shutdown();
	input_.Finalize();
	// dx_ はデストラクタで ImGui
	// などを後始末しているので、そのまま破棄に任せてもよい

	winApp_.Finalize();
	CoUninitialize();
}

void GameApp::Update() {
	// DebugCamera 更新
	if (debugCamera_) {
		debugCamera_->Update(input_);
		view3D_ = debugCamera_->GetViewMatrix();
	} else {
		// フォールバック：今まで通りの固定カメラ
		view3D_ = Inverse(MakeAffineMatrix(
			{ 1.0f, 1.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, -10.0f }));
	}

	// ← ここを if/else の外に出して、毎フレーム共通で計算
	proj3D_ = MakePerspectiveFovMatrix(
		0.45f,
		float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
		0.1f, 100.0f);

	// ===== ImGui フレーム開始 =====
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ===== Settings ウィンドウ =====
	static bool settingsOpen = true;
	if (settingsOpen) {
		ImGui::Begin("Settings", &settingsOpen);

		// Sprite Blend mode
		const char* blendModeItems[] = {
			"Alpha (通常)",    "Add (加算)",          "Subtract (減算)",
			"Multiply (乗算)", "Screen (スクリーン)",
		};
		ImGui::Combo("Sprite Blend", &spriteBlendMode_, blendModeItems,
			IM_ARRAYSIZE(blendModeItems));

		// ===== Particles =====
		ImGui::Text("Particles");

		ImGui::SliderInt("Count", reinterpret_cast<int*>(&particleCountUI_), 0,
			static_cast<int>(kParticleCount_));

		ImGui::DragFloat3("Spawn Center",
			reinterpret_cast<float*>(&particleSpawnCenter_), 0.1f);
		ImGui::DragFloat3("Spawn Extent",
			reinterpret_cast<float*>(&particleSpawnExtent_), 0.1f,
			0.0f, 100.0f);

		ImGui::DragFloat3("Base Dir", reinterpret_cast<float*>(&particleBaseDir_),
			0.01f, -1.0f, 1.0f);
		ImGui::SliderFloat("Dir Random", &particleDirRandomness_, 0.0f, 1.0f);

		ImGui::SliderFloat("Speed Min", &particleSpeedMin_, 0.0f, 10.0f);
		ImGui::SliderFloat("Speed Max", &particleSpeedMax_, 0.0f, 10.0f);
		if (particleSpeedMin_ > particleSpeedMax_)
			particleSpeedMin_ = particleSpeedMax_;

		ImGui::SliderFloat("Life Min", &particleLifeMin_, 0.1f, 10.0f);
		ImGui::SliderFloat("Life Max", &particleLifeMax_, 0.1f, 10.0f);
		if (particleLifeMin_ > particleLifeMax_)
			particleLifeMin_ = particleLifeMax_;

		const char* particleBlendItems[] = {
			"Alpha (通常)", "Add (加算)", "Subtract (減算)",
			"Multiply (乗算)", "Screen (スクリーン)"
		};
		ImGui::Combo("Particle Blend", &particleBlendMode_,
			particleBlendItems, IM_ARRAYSIZE(particleBlendItems));

		ImGui::End();
	}

	// ===== パーティクル行列更新 =====
	if (particleMatrices_) {
		// deltaTime（ちゃんと測るなら前フレーム時刻を記憶する）
		float deltaTime = 1.0f / 60.0f;

		uint32_t activeCount = std::min(particleCountUI_, kParticleCount_);

		for (uint32_t i = 0; i < activeCount; ++i) {
			Particle& p = particles_[i];

			// 時間経過
			p.age += deltaTime;
			if (p.age >= p.lifetime) {
				// 寿命を超えたら再スポーン（色もランダムに変わる）
				RespawnParticle_(p);
			}

			// 位置更新
			p.transform.translate.x += p.velocity.x * deltaTime;
			p.transform.translate.y += p.velocity.y * deltaTime;
			p.transform.translate.z += p.velocity.z * deltaTime;

			// フェードアウト (age/lifetime で 1→0)
			float t = p.age / p.lifetime;
			float alpha = std::clamp(1.0f - t, 0.0f, 1.0f);

			// 行列計算
			Matrix4x4 world = MakeBillboardMatrix(p.transform.scale,
				p.transform.translate, view3D_);
			Matrix4x4 wvp = Multiply(world, Multiply(view3D_, proj3D_));

			particleMatrices_[i].World = world;
			particleMatrices_[i].WVP = wvp;

			// ★ 色 × アルファをインスタンスバッファに書き込む
			particleMatrices_[i].color = {
				p.color.x, p.color.y, p.color.z,
				alpha // ここにフェード用アルファ
			};
		}
	}

	// 最後に ImGui を確定
	ImGui::Render();
}

void GameApp::Draw() {
	dx_.BeginFrame();
	auto* cmdList = dx_.GetCommandList();

	// ===== クリア色 =====
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };

	// DSV
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		dx_.GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

	// 現在のバックバッファRTV
	UINT backBufferIndex = dx_.GetSwapChain()->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx_.GetRTVHandle(backBufferIndex);

	// RTV / DSV 設定
	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// クリア
	cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
		nullptr);

	// ビューポート / シザー
	cmdList->RSSetViewports(1, &dx_.GetViewport());
	cmdList->RSSetScissorRects(1, &dx_.GetScissorRect());

	// ==============================
	// 3D モデル描画（model_, planeModel_）
	// ==============================

	// カメラ行列（とりあえず DebugCamera はまだ使わず固定）
	Matrix4x4 viewMatrix = view3D_;
	Matrix4x4 projectionMatrix = proj3D_;

	// ワールド行列を更新
	Matrix4x4 worldSphere = MakeAffineMatrix(transform_.scale, transform_.rotate,
		transform_.translate);
	model_.SetWorldTransform(worldSphere);
	model_.SetLightingMode(lightingMode_);

	Matrix4x4 worldPlane = MakeAffineMatrix(transform2_.scale, transform2_.rotate,
		transform2_.translate);
	planeModel_.SetWorldTransform(worldPlane);

	// ===== 平行光（InitResources_ で作った CB を使う） =====
	{
		// 必要ならここで色・方向・強さを更新（今は固定値のままでもOK）
		// directionalLightData_->direction = {...}; など

		// Object 用 PSO/RootSignature セット
		cmdList->SetGraphicsRootSignature(objPipeline_.GetRootSignature());
		cmdList->SetPipelineState(objPipeline_.GetPipelineState());

		// モデル描画
		model_.Draw(viewMatrix, projectionMatrix, directionalLightCB_.Get());
		planeModel_.Draw(viewMatrix, projectionMatrix,
			directionalLightCB_.Get());
	}

	// ==============================
  // パーティクル描画（instancing）
  // ==============================
	{
		// どのパイプラインを使うか選ぶ
		UnifiedPipeline* currentParticlePipeline = &particlePipelineAlpha_;
		switch (particleBlendMode_) {
		case 1: currentParticlePipeline = &particlePipelineAdd_;     break;
		case 2: currentParticlePipeline = &particlePipelineSub_;     break;
		case 3: currentParticlePipeline = &particlePipelineMul_;     break;
		case 4: currentParticlePipeline = &particlePipelineScreen_;  break;
		}

		cmdList->SetGraphicsRootSignature(currentParticlePipeline->GetRootSignature());
		cmdList->SetPipelineState(currentParticlePipeline->GetPipelineState());

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &particleVBView_);
		cmdList->IASetIndexBuffer(&particleIBView_);

		ID3D12DescriptorHeap* heaps[] = { dx_.GetSRVHeap() };
		cmdList->SetDescriptorHeaps(1, heaps);

		cmdList->SetGraphicsRootConstantBufferView(
			0, particleMaterialCB_->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(1, particleTextureHandle_);
		cmdList->SetGraphicsRootDescriptorTable(2, particleMatricesSrvGPU_);

		UINT indexCount = 6;
		UINT instanceCount = std::min(particleCountUI_, kParticleCount_);
		cmdList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
	}

	// ==============================
	// 2D スプライト描画
	// ==============================

	// 射影 : 画面ピクセル座標系
	Matrix4x4 view2D = MakeIdentity4x4();
	Matrix4x4 proj2D =
		MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth),
			float(WinApp::kClientHeight), 0.0f, 1.0f);

	// スプライトの Transform 反映
	sprite_.SetPosition(transformSprite_.translate);
	sprite_.SetScale(transformSprite_.scale);
	sprite_.SetRotation(transformSprite_.rotate);

	// UV: scale, rotate.z, translate を使って 2D 用のアフィン行列を作る
	Matrix4x4 uvMat = MakeAffineMatrix(
		{ uvTransformSprite_.scale.x, uvTransformSprite_.scale.y, 1.0f },
		{ 0.0f, 0.0f, uvTransformSprite_.rotate.z },
		{ uvTransformSprite_.translate.x, uvTransformSprite_.translate.y, 0.0f });

	sprite_.SetUVTransform(uvMat);

	// 使用するパイプラインを設定（ImGui で選択）
	UnifiedPipeline* currentSpritePipeline = &spritePipelineAlpha_;
	switch (spriteBlendMode_) {
	case 0:
		currentSpritePipeline = &spritePipelineAlpha_;
		break;
	case 1:
		currentSpritePipeline = &spritePipelineAdd_;
		break;
	case 2:
		currentSpritePipeline = &spritePipelineSub_;
		break;
	case 3:
		currentSpritePipeline = &spritePipelineMul_;
		break;
	case 4:
		currentSpritePipeline = &spritePipelineScreen_;
		break;
	}

	sprite_.SetPipeline(currentSpritePipeline);
	// SRV ヒープ設定（Model と共用）
	ID3D12DescriptorHeap* heaps[] = { dx_.GetSRVHeap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	// Sprite 描画
	// sprite_.Draw(view2D, proj2D);

	// ===== ImGui 描画 =====
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

	dx_.EndFrame();
}

void GameApp::InitLogging_() {
	// ログのディレクトリを用意
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

	// 3D オブジェクト用
	{
		auto desc = UnifiedPipeline::MakeObject3DDesc();
		bool ok = objPipeline_.Initialize(device, dxcUtils, dxcCompiler,
			includeHandler, desc);
		assert(ok);
	}

	// Sprite 用（各種ブレンドモード）
	PipelineDesc sprBase = UnifiedPipeline::MakeSpriteDesc();

	auto sprAlpha = sprBase;
	sprAlpha.blendMode = BlendMode::Alpha;
	assert(spritePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, sprAlpha));

	auto sprAdd = sprBase;
	sprAdd.blendMode = BlendMode::Add;
	assert(spritePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, sprAdd));

	auto sprSub = sprBase;
	sprSub.blendMode = BlendMode::Subtract;
	assert(spritePipelineSub_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, sprSub));

	auto sprMul = sprBase;
	sprMul.blendMode = BlendMode::Multiply;
	assert(spritePipelineMul_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, sprMul));

	auto sprScr = sprBase;
	sprScr.blendMode = BlendMode::Screen;
	assert(spritePipelineScreen_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, sprScr));

	// Particle 用
	auto partDesc = UnifiedPipeline::MakeParticleDesc();

	// ===== Particle 用（ブレンド違いで複数） =====
	PipelineDesc partBase = UnifiedPipeline::MakeParticleDesc();

	auto pAlpha = partBase;
	pAlpha.blendMode = BlendMode::Alpha;
	assert(particlePipelineAlpha_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, pAlpha));

	auto pAdd = partBase;
	pAdd.blendMode = BlendMode::Add;
	assert(particlePipelineAdd_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, pAdd));

	auto pSub = partBase;
	pSub.blendMode = BlendMode::Subtract;
	assert(particlePipelineSub_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, pSub));

	auto pMul = partBase;
	pMul.blendMode = BlendMode::Multiply;
	assert(particlePipelineMul_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, pMul));

	auto pScr = partBase;
	pScr.blendMode = BlendMode::Screen;
	assert(particlePipelineScreen_.Initialize(device, dxcUtils, dxcCompiler,
		includeHandler, pScr));
}

void GameApp::InitResources_() {
	ComPtr<ID3D12Device> device;
	dx_.GetDevice()->QueryInterface(IID_PPV_ARGS(&device));

	// ===== モデル =====
	{
		Model::CreateInfo ci{};
		ci.dx = &dx_;
		ci.pipeline = &objPipeline_;
		ci.srvAlloc = &srvAlloc_;
		ci.modelData = LoadObjFile("resources/sphere", "sphere.obj");
		ci.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		ci.lightingMode = 1;
		bool okModel = model_.Initialize(ci);
		assert(okModel);
	}
	{
		Model::CreateInfo ci2{};
		ci2.dx = &dx_;
		ci2.pipeline = &objPipeline_;
		ci2.srvAlloc = &srvAlloc_;
		ci2.modelData = LoadObjFile("resources/plane", "plane.obj");
		ci2.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		ci2.lightingMode = 1;
		bool okPlane = planeModel_.Initialize(ci2);
		assert(okPlane);
	}

	// ===== Sprite =====
	{
		Sprite::CreateInfo sprInfo{};
		sprInfo.dx = &dx_;
		sprInfo.pipeline = &spritePipelineAlpha_;
		sprInfo.srvAlloc = &srvAlloc_;
		sprInfo.texturePath = "resources/uvChecker.png";
		sprInfo.size = { 640.0f, 360.0f };
		sprInfo.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool okSprite = sprite_.Initialize(sprInfo);
		assert(okSprite);
	}

	// ===== Particle Instancing Buffer =====
	{
		const uint32_t kParticleInstanceCount = kParticleCount_;

		particleInstanceBuffer_ = CreateBufferResource(
			device, sizeof(ParticleForGPU) * kParticleInstanceCount);

		particleInstanceBuffer_->Map(0, nullptr,
			reinterpret_cast<void**>(&particleMatrices_));
		for (uint32_t i = 0; i < kParticleInstanceCount; ++i) {
			particleMatrices_[i].WVP = MakeIdentity4x4();
			particleMatrices_[i].World = MakeIdentity4x4();
			particleMatrices_[i].color = particles_[i].color;
		}

		// SRV 作成（VS:t1 用 StructuredBuffer）
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.NumElements = kParticleInstanceCount;
		desc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
		desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		UINT index = srvAlloc_.Allocate();
		device->CreateShaderResourceView(particleInstanceBuffer_.Get(), &desc,
			srvAlloc_.Cpu(index));
		particleMatricesSrvGPU_ = srvAlloc_.Gpu(index);
	}

	// パーティクル Transform 初期化
	for (uint32_t i = 0; i < kParticleCount_; ++i) {
		RespawnParticle_(particles_[i]);
	}

	// ===== Particle Material CB (PS:b0) =====
	{
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = sizeof(ParticleMaterialData);
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		HRESULT hr = device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&particleMaterialCB_));
		assert(SUCCEEDED(hr));

		particleMaterialCB_->Map(0, nullptr,
			reinterpret_cast<void**>(&particleMaterialData_));

		// 初期値: 白＋アルファ1、ライティング無効、UV=単位行列
		particleMaterialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		particleMaterialData_->enableLighting = 0;
		particleMaterialData_->pad[0] = particleMaterialData_->pad[1] =
			particleMaterialData_->pad[2] = 0.0f;
		particleMaterialData_->uvTransform = MakeIdentity4x4();
	}

	// ===== パーティクル専用テクスチャ読み込み =====
	{
		// 1) 画像を読み込んで mip 生成
		DirectX::ScratchImage mipImages =
			LoadTexture("resources/particle/circle.png");
		const DirectX::TexMetadata& meta = mipImages.GetMetadata();

		// 2) GPU テクスチャリソースを作成
		particleTexture_ = CreateTextureResource(device, meta);
		UploadTextureData(particleTexture_, mipImages);

		// 3) SRV を作成して SRV ヒープに登録
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = meta.format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(meta.mipLevels);

		UINT texIndex = srvAlloc_.Allocate();
		device->CreateShaderResourceView(particleTexture_.Get(), &srvDesc,
			srvAlloc_.Cpu(texIndex));

		// 4) GPU ハンドルを保持（描画時に使う）
		particleTextureHandle_ = srvAlloc_.Gpu(texIndex);
	}

	// ===== DirectionalLight 用 CB (PS:b1) =====
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
			&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&directionalLightCB_));
		assert(SUCCEEDED(hr));

		directionalLightCB_->Map(0, nullptr,
			reinterpret_cast<void**>(&directionalLightData_));

		// 初期値
		*directionalLightData_ = { {1, 1, 1, 1}, {0, -1, 0}, 1.0f };
	}

	// サウンド読み込み
	bool select = audio_.Load("select", L"resources/sound/select.mp3", 1.0f);
	assert(select);
	selectVol_ = 1.0f;

	// 2x2 の単位板ポリ（テクスチャは 0..1）
	struct Vtx {
		float px, py, pz;
		float u, v;
	};
	Vtx quad[4] = {
		{-0.5f, 0.5f, 0.0f, 0.0f, 0.0f},
		{0.5f, 0.5f, 0.0f, 1.0f, 0.0f},
		{-0.5f, -0.5f, 0.0f, 0.0f, 1.0f},
		{0.5f, -0.5f, 0.0f, 1.0f, 1.0f},
	};
	uint16_t idx[6] = { 0, 1, 2, 2, 1, 3 };

	// VB
	ComPtr<ID3D12Resource> particleVB, particleIB;
	{
		auto vbSize = sizeof(quad);
		D3D12_HEAP_PROPERTIES heap{ D3D12_HEAP_TYPE_UPLOAD };
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = vbSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		HRESULT hr = device->CreateCommittedResource(
			&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&particleVB));
		assert(SUCCEEDED(hr));
		void* mapped = nullptr;
		particleVB->Map(0, nullptr, &mapped);
		memcpy(mapped, quad, vbSize);
		particleVB->Unmap(0, nullptr);
		particleVBView_.BufferLocation = particleVB->GetGPUVirtualAddress();
		particleVBView_.StrideInBytes = sizeof(Vtx);
		particleVBView_.SizeInBytes = static_cast<UINT>(vbSize);
		particleVB_ = particleVB; // メンバに保持
	}
	// IB
	{
		auto ibSize = sizeof(idx);
		D3D12_HEAP_PROPERTIES heap{ D3D12_HEAP_TYPE_UPLOAD };
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = ibSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		HRESULT hr = device->CreateCommittedResource(
			&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&particleIB));
		assert(SUCCEEDED(hr));
		void* mapped = nullptr;
		particleIB->Map(0, nullptr, &mapped);
		memcpy(mapped, idx, ibSize);
		particleIB->Unmap(0, nullptr);
		particleIBView_.BufferLocation = particleIB->GetGPUVirtualAddress();
		particleIBView_.Format = DXGI_FORMAT_R16_UINT;
		particleIBView_.SizeInBytes = static_cast<UINT>(ibSize);
		particleIB_ = particleIB; // メンバに保持
	}
}

void GameApp::InitCamera_() {
	// Transform の初期値（main.cpp の初期化を踏襲）
	transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	cameraTransform_ = {
		{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f} };

	transformSprite_ = {
		{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	uvTransformSprite_ = {
		{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	transform2_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f} };

	// DebugCamera 生成（クラス定義に合わせて後で include & new）
	debugCamera_ = new DebugCamera();
}

// GameApp のプライベートメンバ関数として追加してOK
void GameApp::RespawnParticle_(Particle& p) {
	// 位置: 生成中心 ± Extent * ランダム[-1,1]
	auto rndSigned = []() { return Rand01() * 2.0f - 1.0f; };

	Vector3 offset{
		rndSigned() * particleSpawnExtent_.x,
		rndSigned() * particleSpawnExtent_.y,
		rndSigned() * particleSpawnExtent_.z,
	};
	p.transform.translate = {
		particleSpawnCenter_.x + offset.x,
		particleSpawnCenter_.y + offset.y,
		particleSpawnCenter_.z + offset.z,
	};

	// スケール（ここはとりあえず固定。ImGui
	// から調整したければ後でパラメータ追加）
	p.transform.scale = { 0.5f, 0.5f, 0.5f };
	p.transform.rotate = { 0.0f, 0.0f, 0.0f };

	// 方向: BaseDir ± ランダムなノイズ
	Vector3 dir = particleBaseDir_;
	dir.x += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
	dir.y += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
	dir.z += (Rand01() * 2.0f - 1.0f) * particleDirRandomness_;
	if (Length(dir) > 0.0001f) {
		dir = Normalize(dir);
	} else {
		dir = { 0.0f, 1.0f, 0.0f }; // デフォルト上向き
	}

	// 速度
	float speed = RandRange(particleSpeedMin_, particleSpeedMax_);
	p.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };

	// 寿命と年齢
	p.lifetime = RandRange(particleLifeMin_, particleLifeMax_);
	p.age = 0.0f;

	p.color = {
		Rand01(), // R
		Rand01(), // G
		Rand01(), // B
		1.0f      // A
	};
}

// （ヘルパ追加）パーティクル用ビルボード行列生成
Matrix4x4 GameApp::MakeBillboardMatrix(const Vector3& scale,
	const Vector3& translate,
	const Matrix4x4& viewMatrix) {
	Matrix4x4 camWorld = Inverse(viewMatrix);
	Vector3 right{ camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2] };
	Vector3 up{ camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2] };
	Vector3 forward{ camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2] };
	// カメラへ向けるため forward を反転
	forward = { -forward.x, -forward.y, -forward.z };

	Matrix4x4 m{};
	m.m[0][0] = scale.x * right.x;
	m.m[0][1] = scale.x * right.y;
	m.m[0][2] = scale.x * right.z;
	m.m[0][3] = 0;
	m.m[1][0] = scale.y * up.x;
	m.m[1][1] = scale.y * up.y;
	m.m[1][2] = scale.y * up.z;
	m.m[1][3] = 0;
	m.m[2][0] = scale.z * forward.x;
	m.m[2][1] = scale.z * forward.y;
	m.m[2][2] = scale.z * forward.z;
	m.m[2][3] = 0;
	m.m[3][0] = translate.x;
	m.m[3][1] = translate.y;
	m.m[3][2] = translate.z;
	m.m[3][3] = 1;
	return m;
}