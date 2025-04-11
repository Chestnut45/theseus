#pragma once
//-----------------------------------------------------------------------------
// File:			W_Camera.h
// Original Author:	Youssef Ashraf
// Modifications: D'Anyil Landry
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

    // Create a camera with the given view dimensions
    Camera2D(float viewWidth, float viewHeight);
    ~Camera2D();
    
    // View setters
    void SetViewSize(float width, float height);
    void SetPosition(const glm::vec2& position);
    void SetZoom(float zoom);

    // View getters
    inline const glm::vec2& GetViewSize() const { return m_viewSize; }
    inline const glm::vec2& GetPosition() const { return m_position; }
    inline float GetZoom() const { return m_zoom; }

    // Returns a const reference to the combined view-projection matrix of the camera
    const glm::mat4& GetMatrix() const;

    // Sets the speed at which this camera will follow game objects
    inline void SetFollowSpeed(float speed) { m_followSpeed = speed; }

    // Gets the speed at which this camera will follow game objects
    inline float GetFollowSpeed() const { return m_followSpeed; }

    // Binds the camera's uniform buffer to the given index of GL_UNIFORM_BUFFER
    // Updates the UBO if necessary
    void Bind(int index = 0);

    // Constants
    static const inline float MAX_ZOOM = 8.0f;
    static const inline float MIN_ZOOM = 0.0001f;

private:

    // Internal functions
    void _UpdateMatrix() const;
    void _UpdateUBO() const;

    // View data
    glm::vec2 m_viewSize;
    glm::vec2 m_position;
    float m_zoom;

    // Speed at which the camera follows a game object
    float m_followSpeed = 1.0f;

    // Combined view/projection matrix data
    mutable glm::mat4 m_viewProjectionMatrix;
    mutable bool m_needsMatrixUpdate;

    // Uniform buffer object ID
    GLuint m_ubo;
    mutable bool m_needsUBOUpdate;
};
}

#endif // W_CAMERA2D_H