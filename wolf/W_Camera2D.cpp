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

    // Create the uniform buffer object
    glGenBuffers(1, &m_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), &m_projectionMatrix[0], GL_STREAM_DRAW);
}

Camera2D::~Camera2D()
{
    glDeleteBuffers(1, &m_ubo);
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

void Camera2D::Bind(int index)
{
    // Bind the uniform buffer
    glBindBufferBase(GL_UNIFORM_BUFFER, index, m_ubo);

    // Write our current matrix to it
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &m_projectionMatrix[0]);
}

void Camera2D::UpdateMatrix()
{
    // Update the orthographic projection matrix with the camera position and zoom
    m_projectionMatrix = glm::ortho(0.0f, m_screenWidth / m_zoom, 0.0f, m_screenHeight / m_zoom);
    m_projectionMatrix = glm::translate(m_projectionMatrix, glm::vec3(-m_position.x, -m_position.y, 0.0f));
    m_needsMatrixUpdate = false;
}
}