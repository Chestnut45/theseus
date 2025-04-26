#pragma once

//-----------------------------------------------------------------------------
// File:            WeaponItem.h
// Original Author: Aurora Ryder
//
// A class representing the item version of a weapon
//
// Note that all weapons CAN have projectiles associated with them,
// but that does not mean that all weapons SHOOT projectiles.
//-----------------------------------------------------------------------------

#include "EquipmentItem.h"

// Types of weapons
enum class WeaponType {
    SWORD,
    SPEAR,
    BOW,
    DIVINE
};

// Struct to hold the properties of a projectile that a weapon can shoot
struct ProjectileProperties {
    float fDamage; // How much damage does the projectile do?

    glm::vec2 v2HurtboxSize; // How big is the projectile's hurtbox?
    glm::vec2 v2Velocity; // What is the projectile's initial velocity?

    std::string strPathToSprite; // Where is the projectile's sprite located?
};

class WeaponItem : public EquipmentItem {
    public:
        WeaponItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, int p_iTextureFrameIndex, Rarity p_enRarity, WeaponType p_enType, float p_fDelay, float p_fDamage, const glm::vec2& p_v2HurtboxSize, bool p_bHasProjectiles)
            : EquipmentItem(p_enID, p_strName, p_strDesc, p_iValue, p_iTextureFrameIndex, p_enRarity, WEAPON), m_enType(p_enType), m_fDelay(p_fDelay), m_fDamage(p_fDamage), m_v2HurtboxSize(p_v2HurtboxSize), m_bHasProjectiles(p_bHasProjectiles)
            {};

        virtual void SetEquipped(bool p_bEquip);

        // Once you set the type you can't change it
        WeaponType GetWeaponType() const {return m_enType;};

        // Having the ability to shoot projectiles ->
        void SetHasProjectiles(bool p_bHasProjectiles) {m_bHasProjectiles = p_bHasProjectiles;};
        bool GetHasProjectiles() const {return m_bHasProjectiles;};

        // -> Is different than having permission to shoot them
        void ToggleProjectiles(bool p_bToggle) {m_bProjectilesEnabled = p_bToggle;};
        bool GetProjectilesEnabled() const {return m_bProjectilesEnabled;};

        // This is the "cooldown" between attacks
        void SetDelay(float p_fDelay) {m_fDelay = p_fDelay;};
        float GetDelay() const {return m_fDelay;};

        // How much damage does the weapon itself do
        void SetDamage(float p_fDamage) {m_fDamage = p_fDamage;};
        float GetDamage() const {return m_fDamage;};

        // How big is the hurtbox?
        void SetHurtBoxSize(const glm::vec2& p_v2HurtBoxSize) {m_v2HurtboxSize = p_v2HurtBoxSize;};
        glm::vec2 GetHurtBoxSize() const {return m_v2HurtboxSize;};

        // Note that setting the projectile properties will only actually have an effect if/when this weapon uses them, otherwise, you're just filling a struct for fun
        bool SetProjectileProperties(float p_fDamage, const glm::vec2& p_v2HurtBoxSize, const glm::vec2& p_v2Velocity, const std::string& p_strPathToSprite);
        ProjectileProperties GetProjectileProperties() const {return m_Projectile;};

        // Constructs a string containing all of the item details that will be displayed in the on-hover GUI tooltip
        inline virtual std::string GetToolTipText() const {
            // Get the base tooltip text, then add the weapon damage and cooldown time
            std::string strBaseText = EquipmentItem::GetToolTipText() 
                + "\nWeapon Damage: " + std::format("{:.2f}", m_fDamage)
                + "\nCooldown: " + std::format("{:.2f}", m_fDelay);

            // Then, if the item can shoot projectiles
            if (m_bHasProjectiles) {
                // List the projectile properties
                strBaseText += "\nProjectile Damage: " + std::format("{:.2f}", m_Projectile.fDamage);
            }

            // Then retrn the constructed string
            return strBaseText;
        };

    private:
        bool m_bHasProjectiles; // Does this weapon have projectiles associated with it?
        bool m_bProjectilesEnabled = false; // And can this weapon CURRENTLY fire said projectiles? (Default is NO)

        float m_fDelay; // How long do we have to wait before we can attack again
        float m_fDamage; // How much damage do we do?

        glm::vec2 m_v2HurtboxSize; // How big is the hurtbox for this weapon? Note: this is different than a projectile's hurtbox

        WeaponType m_enType; // What type of weapon is this?
        ProjectileProperties m_Projectile; // Struct where we can store projectile properties if/when we need them
};

// Event for when we equip a weapon
struct WeaponEquippedEvent {
    WeaponItem* pWeapon = nullptr; // Whatever weapon was just equipped
};

struct WeaponUnequippedEvent {
    WeaponItem* pWeapon = nullptr; // Whatever weapon was just equipped
};