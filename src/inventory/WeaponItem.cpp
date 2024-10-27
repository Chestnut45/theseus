#include "WeaponItem.h"

// Because we want to send a weapon event when we equip a weapon, we overload the equip method
void WeaponItem::SetEquipped(bool p_bEquip) {
    m_bEquipped = p_bEquip;

    // And if we just equipped something
    if (m_bEquipped) {
        // We let anyone interested know what was just equipped
        wolf::EventManager::TriggerEvent(WeaponEquippedEvent(this));
    }
};

// The weapon's ProjectileProperties will only be set if the weapon HAS projectiles. It doesn't matter if the weapon is currently able to shoot them, though
bool WeaponItem::SetProjectileProperties(float p_fDamage, const glm::vec2& p_v2HurtBoxSize, const glm::vec2& p_v2Velocity, const std::string& p_strPathToSprite) {
    // If this weapon has projectiles
    if (m_bHasProjectiles) {
        // Update the properties and return true
        m_Projectile = ProjectileProperties(p_fDamage, p_v2HurtBoxSize, p_v2Velocity, p_strPathToSprite);
        return true;
    }
    // Otherwise, don't change anything and return false
    return false;
};