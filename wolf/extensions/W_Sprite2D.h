#pragma once

//-----------------------------------------------------------------------------
// File:			W_Sprite2D.h
// Original Author:	D'Anyil Landry
//
// A class representing a renderable 2D sprite component.
// 
// Sprites can have an origin point other than [0, 0] (The bottom-left corner)
//-----------------------------------------------------------------------------

#include <string>

#include "W_ProgramManager.h"
#include "W_Texture.h"
#include "W_VertexBuffer.h"
#include "W_VertexDeclaration.h"
#include "W_GameObject.h"

namespace wolf
{

class Sprite2D : public wolf::BaseComponent
{

// Public implementation
public:

    // Create an empty sprite component
    Sprite2D();

    // Create a sprite component with a texture loaded from the given path
    Sprite2D(const std::string& texturePath);

    ~Sprite2D();

    // Delete copy constructor/assignment
    Sprite2D(const Sprite2D&) = delete;
    Sprite2D& operator=(const Sprite2D&) = delete;

    // Delete move constructor/assignment
    Sprite2D(Sprite2D&& other) = delete;
    Sprite2D& operator=(Sprite2D&& other) = delete;

    // Updates the texture to the one at the given file path
    void SetTexture(const std::string& texturePath);

    // Gets a pointer to the texture this sprite uses
    inline wolf::Texture* GetTexture() const { return m_pTexture; }

    // Set or get the origin to render the sprite from
    // Measured in pixel coordinates from the bottom-left of the texture
    void SetOrigin(const glm::vec2& origin);
    inline const glm::vec2& GetOrigin() const { return m_origin; }

    // Helper to center a sprite to its texture
    // This affects where the sprite will be rendered when using Draw()
    void SetOriginToCenterOfTexture();

    // Set the tint of the sprite with a floating point [0, 1] RGB color
    void SetTint(const glm::vec3& tint) { m_tint = tint; }
    const glm::vec3& GetTint() const { return m_tint; }

    // Set the layer of this sprite
    // 0 is the bottommost layer
    void SetLayer(int layer) { m_layer = layer; }
    int GetLayer() const { return m_layer; }

    bool IsVisible() const { return m_visible; }
    void SetVisibility(bool visible) { m_visible = visible; }

    // Whether or not this sprite should have lighting applied when rendered
    bool IsLightingEnabled() const { return m_lightingEnabled; }
    void SetLightingEnabled(bool value) { m_lightingEnabled = value; }

    // Draw the sprite at the given position, rotation, and scale in world space
    // Multiplies final pixel color by provided tint color
    // NOTE: Requires a Camera2D to be bound to slot 0 before drawing.
    void Draw(const glm::vec2& position, float rotationRadians = 0.0f, const glm::vec2& scale = glm::vec2(1.0f), const glm::vec3& tint = glm::vec3(-1.0f));

    // TODO: Draw the sprite at the given screen position (independent of camera's view, only projection)

// Implementaton
private:

    // Internal functions
    void _IncreaseRefCount();

    // Pointer to the sprite's texture
    wolf::Texture* m_pTexture = nullptr;

    // Origin to draw the sprite at, in pixel coordinates from bottom left
    glm::vec2 m_origin{0.0f};

    // Tint color of the sprite
    glm::vec3 m_tint{1.0f};

    // The layer of the sprite
    // Bottommost layer is 0
    int m_layer = 0;

    bool m_visible = true;
    bool m_lightingEnabled = true;

    // Static resources shared by all sprites
    static inline wolf::Program* s_pProgram = nullptr;
    static inline wolf::VertexBuffer* s_pVertexBuffer = nullptr;
    static inline wolf::IndexBuffer* s_pIndexBuffer = nullptr;
    static inline wolf::VertexDeclaration* s_pVAO = nullptr;

    // Reference counting for resource management
    static inline size_t s_refCount = 0;
};

}