//-----------------------------------------------------------------------------
// File: ColliderComponent.h
// Original Author: Nguyễn Minh Nhật
// Collider.
// User Guide:
//     + Create component
//     + Add at least 1 hitbox for component to function
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

#include "../ColliderManager.h"
#include "../VertexDeclarations.h"

class ColliderManager;

class ColliderComponent : public wolf::BaseComponent
{
    friend ColliderManager;

public:

    enum ColliderType
    {
        HITBOX,
        HURTBOXDD,
        HURTBOXDR,
        HITHURTBOXDD,
        HITHURTBOXDR,
        NONE
    };

    ColliderComponent(ColliderType p_collider_type, bool p_doc , bool p_relativity, wolf::GameObjectID p_ignore_id = -1);   
    virtual ~ColliderComponent();

    // Delete copy constructor/assignment
    ColliderComponent(const ColliderComponent&) = delete;
    ColliderComponent& operator=(const ColliderComponent&) = delete;

    // Delete move constructor/assignment
    ColliderComponent(ColliderComponent&& other) = delete;
    ColliderComponent& operator=(ColliderComponent&& other) = delete;


    void AddColliderBox(glm::vec2 p_dimensions);
    void AddColliderBox(glm::vec2 p_dimensions, glm::vec2 p_offset);

    std::vector<wolf::Rectangle> GetColliderBoxes() const;

    bool IsHitbox() const;
    bool IsHurtbox() const;
    bool IsHurtboxDamageDealer() const;
    bool IsHurtboxDamageReceiver() const;
    bool IsDestroyedOnCollision() const;
    bool IsRelative() const;

    float GetDamage() const;
    ColliderType GetColliderType() const;

    void SetDamage(float p_damage);
    void SetColliderType(ColliderComponent::ColliderType p_collider_type);    

    void SetIgnoreTag(wolf::GameObjectID p_id);

    void FillVertexArray();

    static void DebugDrawAndFlush();
    static int GetComponentCount();
    
private:
    bool m_bIsDestroyedOnCollision = false; // Game object destroyed on collision
    bool m_bIsRelative = false; // Hitbox scales relative to object
    bool m_bIsFlaggedForDestruction = false;
    float m_fDamage = 1.0f; // Amount of Damage to Deal (only for Damage Dealer hurtboxes) 
    wolf::GameObjectID m_IgnoreID = -1; // Projectiles bypass collision check with firer


    ColliderType m_eColliderType = ColliderType::NONE;

    std::vector<wolf::Rectangle> m_vColliderBoxes;

    static int s_iComponentCount;

    static wolf::VertexDeclaration *s_pDecl;
    static wolf::Program *s_pProgram;
    static wolf::VertexBuffer *s_pVB;

    static std::vector<Vertex2D> s_vVerticesVector;
};