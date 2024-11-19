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

AttackDamageComponent::~AttackDamageComponent()
{
    this->m_pColliderManager = nullptr;
}

void AttackDamageComponent::Update(float p_dt)
{
    wolf::GameObject* thisObject = this->GetGameObject();
    ColliderComponent* thisCollider = thisObject->GetComponent<ColliderComponent>();

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
                }
            }
        }
    }
}