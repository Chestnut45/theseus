//-----------------------------------------------------------------------------
// File: VelocityManager.h
// Original Author: Nguyễn Minh Nhật
// Manages velocity components.
// User Guide:
//     + Create new Manager object before any game object is added to scene
//     + Init() to pass reference to scene
//     + Call Update() every frame
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <ranges>
#include <vector>
#include <wolf.h>

#include "components/VelocityComponent.h"

class VelocityManager
{
public:
    VelocityManager(wolf::Scene* p_scene);
    virtual ~VelocityManager();

    void Update(float p_delta);

private:
    wolf::Scene* m_scene = nullptr;     
};