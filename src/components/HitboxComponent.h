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
    HitboxComponent();
    HitboxComponent(glm::vec2 p_dimensions, bool p_doc, bool p_relativity);
    virtual ~HitboxComponent();

    wolf::Rectangle GetHitbox();
    glm::vec2 GetDimensions() const;
    bool IsDestroyedOnCollision() const;
    bool IsRelative() const;

    void FillVertexArray();
    
    static void DebugDrawAndFlush();

private:
    bool IsToBeDestroyed() const;
    void RaiseDestroyFlag();

    wolf::Rectangle m_Hitbox;
    bool m_bIsDestroyedOnCollision = false; // Game object destroyed on collision
    bool m_bDestroy = false; // Game object destruction flag
    bool m_bIsRelative = false;
    
    static int s_iComponentCount;

    static wolf::VertexDeclaration *s_pDecl;
    static wolf::Program *s_pProgram;
    static wolf::VertexBuffer *s_pVB;

    static std::vector<Vertex2D> s_vVerticesVector;   
};