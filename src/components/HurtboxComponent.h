//-----------------------------------------------------------------------------
// File: HurtboxComponent.h
// Original Author: Nguyễn Minh Nhật
// Hurtbox.
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class HurtboxComponent : public wolf::BaseComponent
{
public:  
    HurtboxComponent();
    HurtboxComponent(glm::vec2 p_dimensions, bool p_type, float p_damage, bool p_doc , bool p_relativity);
    

    wolf::Rectangle* GetHurtbox();
    float GetDamage() const;
    glm::vec2 GetDimensions() const;
    bool GetType() const;
    bool IsDestroyedOnCollision() const;
    bool IsRelative() const;
    bool IsToBeDestroyed() const;
    void RaiseDestroyFlag();
    
private:
    wolf::Rectangle * m_pHurtbox = nullptr;
    bool m_bType = 0; // Type - 0: Damage Receiver, 1: Damage Dealer
    float m_iDamage = 0.0f; // Amount of Damage to Deal (only for Damage Dealer hurtboxes) 
    bool m_bIsDestroyedOnCollision = false; // Game object destroyed on collision
    bool m_bDestroy = false; //Game object destruction flag
    bool m_bIsRelative = false;
};