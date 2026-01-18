#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "Audio.h"
#include "ImGuiManager.h"

// マネージャー群
#include "TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"

class AbsoluteFrameWork {
public:
	// コンストラクタ
	AbsoluteFrameWork() = default;
	// 仮想デストラクタ
	virtual ~AbsoluteFrameWork() = default;

	// 実行
	void Run();

	// 終了チェック
	bool IsEndRequested() { return endRequest_; };

protected:
	// 初期化
	virtual void Initialize();
	// 終了
	virtual void Finalize();
	// 毎フレーム更新
	virtual void Update();
	// 描画
	virtual void Draw() = 0;

	// 終了要求（ESCで終わる、ウィンドウ閉じる、などをゲーム側で判定して呼べる）
	void RequestEnd() { endRequest_ = true; }

	// --- エンジン機能へアクセスするためのゲッタ ---
	WinApp& GetWinApp() { return winApp_; }
	DirectXCommon& GetDX() { return dx_; }
	Input& GetInput() { return input_; }
	AudioManager& GetAudio() { return audio_; }
	ImGuiManager& GetImGui() { return imgui_; }

	ID3D12GraphicsCommandList* GetCmdList() { return dx_.GetCommandList(); }
private:
	// エンジンの共通初期化/終了
	void InitializeEngine_();
	void FinalizeEngine_();

private:
	// 終了要求フラグ
	bool endRequest_ = false;

	WinApp winApp_;
	DirectXCommon dx_;
	Input input_;
	AudioManager audio_;
	ImGuiManager imgui_;
};

