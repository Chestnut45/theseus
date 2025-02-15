//-----------------------------------------------------------------------------
// File: PortalTileComponent.h
// Original Author: Nguyễn Minh Nhật
// Portal tile.
// Note: The owner GameObject does NOT need to be moved for this component to work
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>


class PortalTileComponent : public wolf::BaseComponent
{
public:
    PortalTileComponent(glm::ivec2 p_tilePos);
    virtual ~PortalTileComponent();

    void Update(float p_dt);
private:
glm::ivec2 m_vTilePos;
};