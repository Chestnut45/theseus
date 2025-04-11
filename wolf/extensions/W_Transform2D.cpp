//-----------------------------------------------------------------------------
// File:			W_Transform2D.cpp
// Original Author:	D'Anyil Landry
//
// A class representing a 2D transformation component including
// position, rotation, and scale.
//-----------------------------------------------------------------------------

#include "W_Transform2D.h"

#include "W_GameObject.h"

namespace wolf
{

Transform2D::Transform2D()
    : m_position(0.0f, 0.0f),
    m_scale(1.0f, 1.0f),
    m_rotation(0.0f)
{
}

Transform2D::Transform2D(const glm::vec2& position, float rotation, const glm::vec2& scale)
    : m_position(position),
    m_scale(scale),
    m_rotation(rotation)
{
}

Transform2D::~Transform2D()
{
}

glm::mat4 Transform2D::GetLocalMatrix() const
{
    // Initialize with identity matrix
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(m_position, 0.0f));
    transform = glm::rotate(transform, m_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, glm::vec3(m_scale, 1.0f));
    return transform;
}

glm::vec2 Transform2D::GetGlobalPosition() const
{
    GameObject* parent = GetGameObject()->GetParent();
    if (parent)
    {
        Transform2D* parentTransform = parent->GetComponent<Transform2D>();
        if (parentTransform)
        {
            return glm::vec2(parentTransform->GetGlobalMatrix() * glm::vec4(m_position, 0.0f, 1.0f));
        }
    }
    return m_position;
}

float Transform2D::GetGlobalRotation() const
{
    GameObject* parent = GetGameObject()->GetParent();
    if (parent)
    {
        Transform2D* parentTransform = parent->GetComponent<Transform2D>();
        if (parentTransform)
        {
            return parentTransform->GetGlobalRotation() + m_rotation;
        }
    }
    return m_rotation;
}

glm::vec2 Transform2D::GetGlobalScale() const
{
    GameObject* parent = GetGameObject()->GetParent();
    if (parent)
    {
        Transform2D* parentTransform = parent->GetComponent<Transform2D>();
        if (parentTransform)
        {
            return parentTransform->GetGlobalScale() * m_scale;
        }
    }
    return m_scale;
}

glm::mat4 Transform2D::GetGlobalMatrix() const
{
    GameObject* parent = GetGameObject()->GetParent();
    if (parent)
    {
        Transform2D* parentTransform = parent->GetComponent<Transform2D>();
        if (parentTransform)
        {
            return parentTransform->GetGlobalMatrix() * GetLocalMatrix();
        }
    }
    return GetLocalMatrix();
}

}