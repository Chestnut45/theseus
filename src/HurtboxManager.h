//-----------------------------------------------------------------------------
// File: HurtboxManager.h
// Original Author: Nguyễn Minh Nhật
// Manages Hurttbox collision.
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <ranges>
#include <vector>
#include <wolf.h>

#include "components/HealthComponent.h"
#include "components/HurtboxComponent.h"

class HurtboxComponent;

class HurtboxManager
{
public:
    HurtboxManager();
    virtual ~HurtboxManager();

    void Init(wolf::Scene* p_scene);
    void Update();

private:
    wolf::Scene* m_scene = nullptr;

    bool IsColliding(HurtboxComponent* p_hurtboxComponent1, HurtboxComponent* p_hurtboxComponent2);
    void RemoveFlagged();
    void CheckCollisions();

    int m_iComponentCount = 0;

    std::vector<HurtboxComponent *> m_vToBeDestroyed;
};