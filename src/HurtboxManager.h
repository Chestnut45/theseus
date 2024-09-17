//-----------------------------------------------------------------------------
// File: HurtboxManager.h
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Manages Hurttbox collision.
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

#include "components/HealthComponent.h"
#include "components/HurtboxComponent.h"

class HurtboxManager
{
public:
    HurtboxManager();
    virtual ~HurtboxManager();

    void Init(wolf::Scene* p_scene);
    void CheckCollisions();

private:
    wolf::Scene* m_scene = nullptr;

    bool IsColliding(HurtboxComponent* p_hurtbox1, HurtboxComponent* p_hurtbox2);
    
    int count = 0;
};