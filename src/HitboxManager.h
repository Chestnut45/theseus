//-----------------------------------------------------------------------------
// File: HitboxManager.h
// Original Author: Nguyễn Minh Nhật
// Manages Hitbox collision.
// feat. D. Landry
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
public:
    HitboxManager();
    virtual ~HitboxManager();

    void Init(wolf::Scene* p_scene);  
    void Update();

private:
    wolf::Scene* m_scene = nullptr;

    bool IsColliding(HitboxComponent* p_hitbox1, HitboxComponent* p_hitbox2);
    void RemoveFlagged();
    void CheckCollisions();
};

