#pragma once
#include "AbsoluteEngine/scene/Component.h"
#include <string>

/// <summary>
/// FormationSpawnEventで生成された編隊メンバーに付与される軽量な識別コンポーネント。
/// GameScene側が同一formationId_を持つ敵をグループとして追跡し、
/// 編隊全滅ボーナスの判定に使う。JSON経由では生成されず、
/// FormationSpawnEvent::Fire() 内でプログラム的に付与される。
/// </summary>
class FormationMemberComponent : public AbsoluteEngine::IComponent {
public:
    explicit FormationMemberComponent(int formationId = 0) : formationId_(formationId) {}
    ~FormationMemberComponent() override = default;

    std::string GetTypeName() const override { return "FormationMemberComponent"; }

    int GetFormationId() const { return formationId_; }

private:
    int formationId_ = 0;
};
