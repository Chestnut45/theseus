//-----------------------------------------------------------------------------
// File: ProjectileComponent.h
// Original Author: Nguyễn Minh Nhật
// Checks for collision with HURTBOXDRs if owner object has HURTBOXDD.
//-----------------------------------------------------------------------------

#include "AttackDamageComponent.h"

#include "ColliderComponent.h"

AttackDamageComponent::AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager)
{
    this->m_fDamage = p_damage;
    this->m_pColliderManager = p_collider_manager;
}

// Overloaded constructor for knockback
AttackDamageComponent::AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, float knockbackMagnitude)
{
    this->m_fDamage = p_damage;
    this->m_pColliderManager = p_collider_manager;
    this->m_knockbackMagnitude = knockbackMagnitude;
}

AttackDamageComponent::~AttackDamageComponent()
{
    this->m_pColliderManager = nullptr;
}

void AttackDamageComponent::Update(float p_dt)
{
    wolf::GameObject* thisObject = this->GetGameObject();
    ColliderComponent* thisCollider = thisObject->GetComponent<ColliderComponent>();
    wolf::Transform2D* thisTransform = thisObject->GetComponent<wolf::Transform2D>();


    if (thisCollider != nullptr && thisCollider->IsHurtboxDamageDealer())
    {
        for (auto&&[thatID, thatHealth, thatCollider] : this->GetGameObject()->GetScene().Each<HealthComponent, ColliderComponent>())
        {
            if (thatCollider.IsActive() && thatCollider.IsHurtboxDamageReceiver())
            {
                if (this->m_pColliderManager->IsColliding(*thisCollider, thatCollider, p_dt))
                {
                    // std::cout << "DAMAGE: " << thatHealth.GetHealth() << " - " << m_fDamage << " = ";
                    thatHealth.Damage(m_fDamage);
                    // std::cout << thatHealth.GetHealth() << std::endl;
                    // Apply knockback if magnitude > 0
                    // Apply knockback if magnitude > 0
                    if (m_knockbackMagnitude > 0.0f)
                    {
                        auto* thatTransform = thatCollider.GetGameObject()->GetComponent<wolf::Transform2D>();
                        auto* velocityComponent = thatCollider.GetGameObject()->GetComponent<VelocityComponent>();
                        if (thatTransform && velocityComponent)
                        {
                            glm::vec2 knockbackDirection = glm::normalize(
                                thatTransform->GetGlobalPosition() - thisTransform->GetGlobalPosition()
                            );
                            velocityComponent->ApplyKnockback(knockbackDirection, m_knockbackMagnitude);
                        }
                    }
                }
            }
        }
    }
}