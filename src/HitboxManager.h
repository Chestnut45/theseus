//-----------------------------------------------------------------------------
// File: HitboxManager.h
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Manages Hitbox collision.
// feat. D. Landry
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

#include "components/HitboxComponent.h"

class HitboxManager
{
public:
    HitboxManager();
    virtual ~HitboxManager();

    void Init(wolf::Scene* p_scene);
    void CheckCollisions();

private:
    wolf::Scene* m_scene = nullptr;

    bool IsColliding(HitboxComponent* p_hitbox1, HitboxComponent* p_hitbox2);
    int count = 0;
};

