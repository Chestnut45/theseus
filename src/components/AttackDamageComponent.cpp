#include "AttackDamageComponent.h"
#include "ColliderComponent.h"
#include "../events/KnockbackEvent.h"
#include "W_EventManager.h"

AttackDamageComponent::AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, WeaponType p_weaponType)
    : m_fDamage(p_damage), m_pColliderManager(p_collider_manager), m_weaponType(p_weaponType), m_isWeaponAttack(true) {}

AttackDamageComponent::AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, AttackSourceType p_sourceType)
    : m_fDamage(p_damage), m_pColliderManager(p_collider_manager), m_sourceType(p_sourceType), m_isWeaponAttack(false) {}

AttackDamageComponent::~AttackDamageComponent() {
    m_pColliderManager = nullptr;
}

void AttackDamageComponent::Update(float p_dt) {
    auto* thisObject = this->GetGameObject();
    auto* thisCollider = thisObject->GetComponent<ColliderComponent>();

    if (thisCollider && thisCollider->IsHurtboxDamageDealer()) {
        for (auto&& [thatID, thatHealth, thatCollider] : thisObject->GetScene().Each<HealthComponent, ColliderComponent>()) {
            if (thatCollider.IsActive() && thatCollider.IsHurtboxDamageReceiver()) {
                if (m_pColliderManager->IsColliding(*thisCollider, thatCollider, p_dt)) {
                    // Apply damage
                    thatHealth.Damage(m_fDamage);

                    // Calculate knockback direction
                    glm::vec2 knockbackDirection = glm::normalize(
                        thatCollider.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition() -
                        thisObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition()
                    );

                    // Ensure the knockback direction is not zero-length
                    if (glm::length(knockbackDirection) == 0.0f) {
                        knockbackDirection = glm::vec2(0.0f, 1.0f);  // Default upward direction
                    }

                    // Apply knockback based on weapon or source type
                    ApplyKnockback(thatCollider.GetGameObject(), knockbackDirection);
                }
            }
        }
    }
}

void AttackDamageComponent::ApplyKnockback(wolf::GameObject* target, const glm::vec2& direction) {
    float knockbackForce = 0.0f;

    // Determine knockback force based on whether it's a weapon attack or source type
    if (m_isWeaponAttack) {
        switch (m_weaponType) {
            case WeaponType::SWORD:
                knockbackForce = 150.0f;
                break;
            case WeaponType::BOW:
                knockbackForce = 100.0f;
                break;
            case WeaponType::SPEAR:
                knockbackForce = 130.0f;
                break;
        }
    } else {
        switch (m_sourceType) {
            case AttackSourceType::HARPY_PROJECTILE:
                knockbackForce = 120.0f;
                break;
            case AttackSourceType::THROWABLE_OBJECT_PROJECTILE:
                knockbackForce = 180.0f;
                break;
            default:
                knockbackForce = 50.0f;  // Default knockback for other sources
                break;
        }
    }

    // Trigger KnockbackEvent if force is applied
    if (knockbackForce > 0.0f) {
        wolf::EventManager::TriggerEvent(KnockbackEvent(target, direction, knockbackForce));
    }
}
