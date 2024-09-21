//-----------------------------------------------------------------------------
// File: HitboxComponent.h
// Original Author: Nguyễn Minh Nhật
// Hitbox.
//-----------------------------------------------------------------------------

#pragma once

#include <vector>
#include <wolf.h>

#include "../HitboxManager.h"
#include "../VertexDeclarations.h"

class HitboxManager;

class HitboxComponent : public wolf::BaseComponent
{
    friend HitboxManager;

public:
    // HitboxComponent();
    HitboxComponent(bool p_doc, bool p_relativity);
    virtual ~HitboxComponent();

    // Delete copy constructor/assignment
    HitboxComponent(const HitboxComponent&) = delete;
    HitboxComponent& operator=(const HitboxComponent&) = delete;

    // Delete move constructor/assignment
    HitboxComponent(HitboxComponent&& other) = delete;
    HitboxComponent& operator=(HitboxComponent&& other) = delete;

    void AddHitbox(glm::vec2 p_dimensions);

    std::vector<wolf::Rectangle> GetHitboxes() const;

    bool IsDestroyedOnCollision() const;
    bool IsRelative() const;

    void FillVertexArray();
    
    static void DebugDrawAndFlush();

private:
    bool m_bIsDestroyedOnCollision = false; // Game object destroyed on collision
    bool m_bDestroy = false; // Game object destruction flag
    bool m_bIsRelative = false; // Hitboxes relative to scale of object
    
    std::vector<wolf::Rectangle> m_vHitboxes;
    
    static int s_iComponentCount;

    static wolf::VertexDeclaration *s_pDecl;
    static wolf::Program *s_pProgram;
    static wolf::VertexBuffer *s_pVB;

    static std::vector<Vertex2D> s_vVerticesVector; 

    bool IsToBeDestroyed() const;
    void RaiseDestroyFlag();

    static constexpr auto in_place_delete = true;
};