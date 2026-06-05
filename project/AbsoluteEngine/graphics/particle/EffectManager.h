#pragma once
#include "BaseEffect.h"
#include <list>
#include <memory>

class Camera;

class EffectManager {
public:
    static EffectManager* GetInstance();

    void Initialize();
    void Finalize();

    void Update(float deltaTime, const Camera* camera);
    void Draw();

    void AddEffect(std::unique_ptr<BaseEffect> effect);
    void ClearAll();

private:
    EffectManager() = default;
    ~EffectManager() = default;
    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;

    std::list<std::unique_ptr<BaseEffect>> effects_;
};
