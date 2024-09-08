#pragma once

//-----------------------------------------------------------------------------
// File:			W_Sprite2D.h
// Original Author:	D'Anyil Landry
//
// A class representing a renderable 2D sprite component.
//-----------------------------------------------------------------------------

#include "W_Texture.h"
#include "W_VertexDeclaration.h"
#include "W_VertexBuffer.h"
#include "W_ProgramManager.h"
#include <string>

namespace wolf
{

class Sprite2D
{

// Public implementation
public:

    // Create a sprite component with a texture loaded from the given path
    Sprite2D(const std::string& texturePath);
    ~Sprite2D();

    // Delete copy constructor/assignment
    Sprite2D(const Sprite2D&) = delete;
    Sprite2D& operator=(const Sprite2D&) = delete;

    // Delete move constructor/assignment
    Sprite2D(Sprite2D&& other) = delete;
    Sprite2D& operator=(Sprite2D&& other) = delete;

    // Draw the sprite at the given world position
    // NOTE: Requires a Camera2D to be bound to slot 0 before drawing
    void Draw(const glm::vec2& worldPosition, float rotationDegrees = 0.0f, const glm::vec2& scale = glm::vec2(1.0f), const glm::vec3& color = glm::vec3(1.0f));

    // TODO: Draw the sprite at the given screen position (independent of camera's view, only projection)

    // TODO: Allow sprites to be centered (offset position)

// Implementaton
private:

    // Per-sprite texture pointer
    wolf::Texture* m_pTexture = nullptr;

    // Size of the sprite (width, height in pixels)
    glm::vec2 m_size;

    // Static resources shared by all sprites
    static inline wolf::Program* s_pProgram = nullptr;
    static inline wolf::VertexBuffer* s_pVertexBuffer = nullptr;
    static inline wolf::IndexBuffer* s_pIndexBuffer = nullptr;
    static inline wolf::VertexDeclaration* s_pVAO = nullptr;

    // Reference counting for resource management
    static inline size_t refCount = 0;
};

}