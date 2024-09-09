#pragma once

//-----------------------------------------------------------------------------
// File:			W_Transform2D.h
// Original Author:	D'Anyil Landry
//
// A class representing a 2D transformation component including
// position, rotation, and scale.
//-----------------------------------------------------------------------------

#include "W_Types.h"

namespace wolf
{

class Transform2D
{

// Public interface
public:

    // Create an identity transform
    Transform2D();

    // Create a transform with the given position, rotation, and scale
    Transform2D(const glm::vec2& position, float rotation, const glm::vec2& scale);

    ~Transform2D();

    // Delete copy constructor/assignment
    Transform2D(const Transform2D&) = delete;
    Transform2D& operator=(const Transform2D&) = delete;

    // Delete move constructor/assignment
    Transform2D(Transform2D&& other) = delete;
    Transform2D& operator=(Transform2D&& other) = delete;
    
    void SetPosition(const glm::vec2& position) { m_position = position; }
    void Translate(const glm::vec2& offset) { m_position += offset; }
    const glm::vec2& GetPosition() const { return m_position; }
    
    void SetRotation(float rotation) { m_rotation = rotation; }
    void SetRotationDegrees(float rotationDegrees) { m_rotation = glm::radians(rotationDegrees); }
    void Rotate(float rotation) { m_rotation += rotation; }
    void RotateDegrees(float rotationDegrees) { m_rotation += glm::radians(rotationDegrees); }
    float GetRotation() const { return m_rotation; }
    
    void SetScale(const glm::vec2& scale) { m_scale = scale; }
    void Scale(const glm::vec2& scale) { m_scale *= scale; }
    const glm::vec2& GetScale() const { return m_scale; }

    // Calculates and returns a combined transformation matrix
    glm::mat4 GetMatrix() const;

// Implementation
private:

    glm::vec2 m_position;
    glm::vec2 m_scale;
    float m_rotation;
};

}