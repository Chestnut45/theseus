//-----------------------------------------------------------------------------
// File:			W_Camera.cpp
// Original Author:	Youssef Ashraf
//
// A class that's responsible for the camera.
//-----------------------------------------------------------------------------
#include "W_Camera2D.h"

namespace wolf
{
Camera2D::Camera2D(float screenWidth, float screenHeight)
    : m_position(0.0f, 0.0f),
      m_zoom(1.0f),
      m_needsMatrixUpdate(true),
      m_screenWidth(screenWidth),
      m_screenHeight(screenHeight)
{
    // Initialize the orthographic projection matrix
    m_projectionMatrix = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
}

void Camera2D::SetPosition(const glm::vec2& position)
{
    m_position = position;
    m_needsMatrixUpdate = true;
}

void Camera2D::SetZoom(float zoom)
{
    m_zoom = zoom;
    m_needsMatrixUpdate = true;
}

const glm::mat4& Camera2D::GetProjectionMatrix() const
{
    if (m_needsMatrixUpdate)
    {
        const_cast<Camera2D*>(this)->UpdateMatrix();
    }
    return m_projectionMatrix;
}

void Camera2D::UpdateMatrix()
{
    // Update the orthographic projection matrix with the camera position and zoom
    m_projectionMatrix = glm::ortho(0.0f, m_screenWidth / m_zoom, 0.0f, m_screenHeight / m_zoom);
    m_projectionMatrix = glm::translate(m_projectionMatrix, glm::vec3(-m_position.x, -m_position.y, 0.0f));
    m_needsMatrixUpdate = false;
}
}