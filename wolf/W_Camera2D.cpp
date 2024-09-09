//-----------------------------------------------------------------------------
// File:			W_Camera.cpp
// Original Author:	Youssef Ashraf
// Modifications: D'Anyil Landry
//
// A class that's responsible for the camera.
//-----------------------------------------------------------------------------
#include "W_Camera2D.h"

namespace wolf
{
Camera2D::Camera2D(float viewWidth, float viewHeight)
    : m_viewSize(viewWidth, viewHeight),
      m_position(0.0f, 0.0f),
      m_zoom(1.0f),
      m_needsMatrixUpdate(true),
      m_ubo(0),
      m_needsUBOUpdate(true)
{
    // Initialize the view projection matrix
    _UpdateMatrix();

    // Create the uniform buffer object
    glGenBuffers(1, &m_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), &m_viewProjectionMatrix[0], GL_STREAM_DRAW);
}

Camera2D::~Camera2D()
{
    glDeleteBuffers(1, &m_ubo);
}

void Camera2D::SetViewSize(float width, float height)
{
    m_viewSize.x = width;
    m_viewSize.y = height;
    m_needsMatrixUpdate = true;
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

const glm::mat4& Camera2D::GetMatrix() const
{
    if (m_needsMatrixUpdate) _UpdateMatrix();
    return m_viewProjectionMatrix;
}

void Camera2D::Bind(int index)
{
    if (m_needsMatrixUpdate) _UpdateMatrix();
    if (m_needsUBOUpdate) _UpdateUBO();
    glBindBufferBase(GL_UNIFORM_BUFFER, index, m_ubo);
}

void Camera2D::_UpdateMatrix() const
{
    // Calculate the half-extents of the view
    float halfWidth = m_viewSize.x / m_zoom * 0.5f;
    float halfHeight = m_viewSize.y / m_zoom * 0.5f;

    // Calculate the orthographic projection (centered at the camera's position)
    m_viewProjectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);

    // Calculate the view translation
    m_viewProjectionMatrix = glm::translate(m_viewProjectionMatrix, glm::vec3(-m_position.x, -m_position.y, 0.0f));

    // Update flags
    m_needsMatrixUpdate = false;
    m_needsUBOUpdate = true;
}

void Camera2D::_UpdateUBO() const
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &m_viewProjectionMatrix[0]);
}

}