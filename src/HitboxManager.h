//-----------------------------------------------------------------------------
// File: HitboxManager.h
// Original Author: Nguyễn Minh Nhật
// Manages Hitbox collision.
// User Guide:
//     + Create new Manager object before any game object is added to scene
//     + Init() to pass reference to scene
//     + Call Update() every frame
// Notes:
//     + s_iComponentCount incremented/decremented in HitboxComponent
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <ranges>
#include <wolf.h>

#include "components/HitboxComponent.h"
#include "components/VelocityComponent.h"

class HitboxComponent;
class HitboxManager
{
friend HitboxComponent;

public:
    HitboxManager(wolf::Scene* p_scene);
    virtual ~HitboxManager();

    void Update();

private:
    wolf::Scene* m_scene = nullptr;

    bool IsColliding(HitboxComponent* p_hitboxComponent1, HitboxComponent* p_hitboxComponent2);
    void RemoveFlagged();
    void CheckCollisions();

    std::vector<HitboxComponent *> m_vToBeDestroyed;
};

