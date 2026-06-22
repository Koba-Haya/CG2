#include "BaseScene.h"
#include "SceneManager.h"

// エンジン機能用インクルード
#include "AbsoluteEngine/editor/EditorUIManager.h"
#include "AbsoluteEngine/editor/EditorCamera.h"
#include "AbsoluteEngine/scene/SceneSerializer.h"
#include "GameObject.h"
#include "Input.h"
#include "Renderer.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

void BaseScene::Initialize(const SceneServices &services) {
    services_ = services;

    // カメラの初期化（ゲーム中も使用するためマクロ外で生成）
    editorCamera_ = std::make_unique<AbsoluteEngine::EditorCamera>();
    editorCamera_->Initialize();
    editorCamera_->SetPerspective(0.45f, 16.0f / 9.0f, 0.1f, 1000.0f);

#ifdef USE_IMGUI
    // エディタUIマネージャーの初期化
    editorUIManager_ = std::make_unique<AbsoluteEngine::EditorUIManager>();
#endif
}

void BaseScene::RequestSceneChange(const std::string &sceneId) {
  if (sceneManager_) {
    sceneManager_->RequestChange(sceneId);
  }
}

void BaseScene::UpdateEditor() {
#ifdef USE_IMGUI
    // ドラッグ＆ドロップ中はカメラの操作をブロックする
    bool isDragging = ImGui::GetDragDropPayload() != nullptr;

    // エディタカメラの更新
    if (editorCamera_ && playMode_ == PlayMode::Edit && !isDragging) {
        editorCamera_->Update(*services_.input);
    }
#endif

    // プレイモード中のオブジェクトの更新
    if (playMode_ == PlayMode::Play) {
        const float deltaTime = 1.0f / 60.0f; // 共通のdeltaTimeを使う想定
        for (auto& obj : rootObjects_) {
            if (obj) {
                obj->Update(deltaTime);
            }
        }
    }
}

void BaseScene::DrawEditorUI() {
#ifdef USE_IMGUI
    // ツールバー（プレイモード切り替え）
    ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
    
    if (playMode_ == PlayMode::Edit) {
        if (ImGui::Button("Play")) {
            backupSceneJson_ = AbsoluteEngine::SceneSerializer::SerializeToString(rootObjects_);
            playMode_ = PlayMode::Play;
        }
    } else if (playMode_ == PlayMode::Play) {
        if (ImGui::Button("Pause")) {
            playMode_ = PlayMode::Pause;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            rootObjects_.clear();
            AbsoluteEngine::SceneSerializer::DeserializeFromString(backupSceneJson_, rootObjects_);
            if (editorUIManager_) editorUIManager_->SetSelectedObject(nullptr);
            playMode_ = PlayMode::Edit;
        }
    } else if (playMode_ == PlayMode::Pause) {
        if (ImGui::Button("▶ Resume")) {
            playMode_ = PlayMode::Play;
        }
        ImGui::SameLine();
        if (ImGui::Button("■ Stop")) {
            rootObjects_.clear();
            AbsoluteEngine::SceneSerializer::DeserializeFromString(backupSceneJson_, rootObjects_);
            if (editorUIManager_) editorUIManager_->SetSelectedObject(nullptr);
            playMode_ = PlayMode::Edit;
        }
    }
    ImGui::End();

    // エディタUIの描画（Hierarchy, Inspector, Gizmo）
    if (editorUIManager_ && editorCamera_) {
        auto* edCam = dynamic_cast<AbsoluteEngine::EditorCamera*>(editorCamera_.get());
        editorUIManager_->DrawUI(rootObjects_, editorCamera_->GetViewMatrix(), editorCamera_->GetProjectionMatrix(), edCam);
    }
#endif
}

void BaseScene::ApplyEditorLightsToRenderer(Renderer* renderer) {
    if (!renderer) return;

    std::vector<DirLight> dirLights;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;

    auto collectLights = [&](auto& self, const std::shared_ptr<AbsoluteEngine::GameObject>& obj) -> void {
        if (!obj) return;
        const auto& light = obj->GetLight();
        const auto& t = obj->GetTransform();

        // Transformの回転から方向ベクトルを計算
        Matrix4x4 rotX = MakeRotateXMatrix(t.rotate.x);
        Matrix4x4 rotY = MakeRotateYMatrix(t.rotate.y);
        Matrix4x4 rotZ = MakeRotateZMatrix(t.rotate.z);
        Matrix4x4 rotMatrix = Multiply(Multiply(rotZ, rotX), rotY);
        
        Vector3 defaultDir = {0.0f, -1.0f, 0.0f};
        Vector3 dir = TransformNormal(defaultDir, rotMatrix);
        dir = Normalize(dir);

        if (light.type == AbsoluteEngine::LightComponent::Type::Directional) {
            DirLight dl;
            dl.color = light.color;
            dl.intensity = light.intensity;
            dl.direction = dir;
            dl.enabled = true;
            dirLights.push_back(dl);
        } else if (light.type == AbsoluteEngine::LightComponent::Type::Point) {
            PointLight pl;
            pl.color = light.color;
            pl.intensity = light.intensity;
            pl.radius = light.radius;
            pl.decay = light.decay;
            pl.position = t.translate; 
            pl.enabled = true;
            pointLights.push_back(pl);
        } else if (light.type == AbsoluteEngine::LightComponent::Type::Spot) {
            SpotLight sl;
            sl.color = light.color;
            sl.intensity = light.intensity;
            sl.distance = light.distance;
            sl.decay = light.decay;
            sl.coneAngleDeg = light.coneAngleDeg;
            sl.position = t.translate;
            sl.direction = dir;
            sl.enabled = true;
            spotLights.push_back(sl);
        }

        for (const auto& child : obj->GetChildren()) {
            self(self, child);
        }
    };

    for (const auto& obj : rootObjects_) {
        collectLights(collectLights, obj);
    }

    renderer->SetDirectionalLights(dirLights, true);
    renderer->SetPointLights(pointLights, true);
    renderer->SetSpotLights(spotLights, true);
}
