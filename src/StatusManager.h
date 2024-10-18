//-----------------------------------------------------------------------------
// File: StatusManager.h
// Original Author: Nguyễn Minh Nhật
// Manager for StatusComponent.
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>

#include "components/StatusComponent.h"

class StatusComponent;

class StatusManager
{
public:
    StatusManager(wolf::Scene* p_scene);
    ~StatusManager();

    void Update(float p_delta);

private:
    wolf::Scene* m_scene = nullptr;
};