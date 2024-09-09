#include "W_Transform2D.h"

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

glm::mat4 Transform2D::GetMatrix() const
{
    // Initialize with identity matrix
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(m_position, 0.0f));
    transform = glm::rotate(transform, m_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, glm::vec3(m_scale, 1.0f));
    return transform;
}

}