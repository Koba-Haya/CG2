#include "EffectManager.h"

EffectManager* EffectManager::GetInstance() {
    static EffectManager instance;
    return &instance;
}

void EffectManager::Initialize() {
    ClearAll();
}

void EffectManager::Finalize() {
    ClearAll();
}

void EffectManager::Update(float deltaTime, const Camera* camera) {
    for (auto it = effects_.begin(); it != effects_.end();) {
        (*it)->Update(deltaTime, camera);
        if ((*it)->IsDead()) {
            it = effects_.erase(it);
        } else {
            ++it;
        }
    }
}

void EffectManager::Draw() {
    for (auto& effect : effects_) {
        effect->Draw();
    }
}

void EffectManager::AddEffect(std::unique_ptr<BaseEffect> effect) {
    effects_.push_back(std::move(effect));
}

void EffectManager::ClearAll() {
    effects_.clear();
}
