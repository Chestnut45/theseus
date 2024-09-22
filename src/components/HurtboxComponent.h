//-----------------------------------------------------------------------------
// File: HurtboxComponent.h
// Original Author: Nguyễn Minh Nhật
// Hurtbox.
// User Guide:
//     + Create component
//     + Add at least 1 hitbox for component to function
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

#include "../HurtboxManager.h"
#include "../VertexDeclarations.h"

class HurtboxManager;

class HurtboxComponent : public wolf::BaseComponent
{
    friend HurtboxManager;

public:  
    HurtboxComponent(bool p_type, float p_damage, bool p_doc , bool p_relativity);
    
    virtual ~HurtboxComponent();

    // Delete copy constructor/assignment
    HurtboxComponent(const HurtboxComponent&) = delete;
    HurtboxComponent& operator=(const HurtboxComponent&) = delete;

    // Delete move constructor/assignment
    HurtboxComponent(HurtboxComponent&& other) = delete;
    HurtboxComponent& operator=(HurtboxComponent&& other) = delete;

    void AddHurtbox(glm::vec2 p_dimensions);
    void AddHurtbox(glm::vec2 p_dimensions, glm::vec2 p_offset);

    wolf::Rectangle GetHurtbox();
    std::vector<wolf::Rectangle> GetHurtboxes() const;

    float GetDamage() const;
    bool GetType() const;
    bool IsDestroyedOnCollision() const;
    bool IsRelative() const;

    void FillVertexArray();

    static void DebugDrawAndFlush();
    static int GetComponentCount();
    
private:
    bool m_bType = 0; // Type - 0: Damage Receiver, 1: Damage Dealer
    float m_iDamage = 0.0f; // Amount of Damage to Deal (only for Damage Dealer hurtboxes) 
    bool m_bIsDestroyedOnCollision = false; // Game object destroyed on collision
    bool m_bDestroy = false; //Game object destruction flag
    bool m_bIsRelative = false; // Hitbox scales relative to object

    std::vector<wolf::Rectangle> m_vHurtboxes;

    static int s_iComponentCount;

    static wolf::VertexDeclaration *s_pDecl;
    static wolf::Program *s_pProgram;
    static wolf::VertexBuffer *s_pVB;

    static std::vector<Vertex2D> s_vVerticesVector;

    bool IsToBeDestroyed() const;
    void RaiseDestroyFlag();
};