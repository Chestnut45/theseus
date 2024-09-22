//-----------------------------------------------------------------------------
// File: HurtboxManager.h
// Original Author: Nguyễn Minh Nhật
// Manages Hurtbox collision.
// User Guide:
//     + Create new Manager object before any game object is added to scene
//     + Init() to pass reference to scene
//     + Call Update() every frame
// Notes:
//     + s_iComponentCount incremented/decremented in HurtboxComponent
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
friend HurtboxComponent;

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

    static int s_iComponentCount;

    std::vector<HurtboxComponent *> m_vToBeDestroyed;
};