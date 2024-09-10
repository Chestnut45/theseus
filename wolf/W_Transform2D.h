#pragma once

//-----------------------------------------------------------------------------
// File:			W_Transform2D.h
// Original Author:	D'Anyil Landry
//
// A class representing a 2D transformation component including
// position, rotation, and scale.
//-----------------------------------------------------------------------------

#include "W_Types.h"
#include "W_GameObject.h"

namespace wolf
{

class Transform2D : public BaseComponent
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
    
    void SetRotation(float rotation) { m_rotation = rotation; }
    void SetRotationDegrees(float rotationDegrees) { m_rotation = glm::radians(rotationDegrees); }
    void Rotate(float rotation) { m_rotation += rotation; }
    void RotateDegrees(float rotationDegrees) { m_rotation += glm::radians(rotationDegrees); }
    
    void SetScale(const glm::vec2& scale) { m_scale = scale; }
    void Scale(const glm::vec2& scale) { m_scale *= scale; }

    // Accessors to local transform data
    const glm::vec2& GetLocalPosition() const { return m_position; }
    float GetLocalRotation() const { return m_rotation; }
    const glm::vec2& GetLocalScale() const { return m_scale; }
    glm::mat4 GetLocalMatrix() const;

    // Get the global transform data from the game object hierarchy
    glm::vec2 GetGlobalPosition() const;
    float GetGlobalRotation() const;
    glm::vec2 GetGlobalScale() const;
    glm::mat4 GetGlobalMatrix() const;

// Implementation
private:

    // Transform data
    glm::vec2 m_position;
    glm::vec2 m_scale;
    float m_rotation;
};

}