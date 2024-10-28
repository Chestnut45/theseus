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

    if(thisCollider != nullptr && thisCollider->IsHurtboxDamageDealer())
    {
        for (auto&&[thatID, thatObject, thatCollider] : this->GetGameObject()->GetScene().Each<wolf::GameObject, ColliderComponent>())
        {
            if(thatCollider.IsHurtboxDamageReceiver())
            {
                if(this->m_pColliderManager->IsColliding(thisCollider, &thatCollider, p_dt))
                {
                    HealthComponent* thatHealth = thatObject.GetComponent<HealthComponent>();
                    if(thatHealth != nullptr)
                    {
                        thatHealth->Damage(m_fDamage);
                    }
                }
            }
        }
    }
}