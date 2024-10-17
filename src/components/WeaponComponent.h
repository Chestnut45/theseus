//-----------------------------------------------------------------------------
// File: WeaponComponent.h
// Original Author: Nguyễn Minh Nhật
// Base for weapons.
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class WeaponComponent : public wolf::BaseComponent
{
public:
    enum WeaponType
    {

        BOW,
        CROSSBOW,
        SWORD,
        NONE //Fists
    };

    WeaponComponent();
    
    void Attack();
    void CollectWeapon(WeaponType p_weapon_type);

private:
    // Weapon properties
    float m_fDelay = 0.0f;

    bool m_aAvailableWeapons[WeaponType::NONE];

    // Projectile/Melee properties
    float m_fAtkDamage = 0.0f;
    WeaponType m_CurrentWeapon = WeaponType::NONE;
    glm::vec2 m_vAtkVelocity = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_vAtkHurtboxSize = glm::vec2(1.0f, 1.0f);
    
};