#pragma once
//-----------------------------------------------------------------------------
// File:			W_Camera.h
// Original Author:	Youssef Ashraf
//
// A header for the camera class.
//-----------------------------------------------------------------------------
#ifndef W_CAMERA2D_H
#define W_CAMERA2D_H

#include "W_Types.h"

namespace wolf
{
class Camera2D
{
public:
    Camera2D(float screenWidth, float screenHeight);

    void SetPosition(const glm::vec2& position);
    void SetZoom(float zoom);

    const glm::mat4& GetProjectionMatrix() const;

private:
    void UpdateMatrix();

    glm::vec2 m_position;
    float m_zoom;
    glm::mat4 m_projectionMatrix;
    bool m_needsMatrixUpdate;

    float m_screenWidth;
    float m_screenHeight;
};
}

#endif // W_CAMERA2D_H