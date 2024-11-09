//-----------------------------------------------------------------------------
// File: ProjectileComponent.h
// Original Author: Nguyễn Minh Nhật
// Checks for collision with HURTBOXDRs if owner object has HURTBOXDD.
//-----------------------------------------------------------------------------

#include "AttackDamageComponent.h"
#include "ColliderComponent.h"

AttackDamageComponent::AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, std::vector<std::pair<StatusComponent::StatusEffectType, float>> p_status_effects)
{
    this->m_fDamage = p_damage;
    this->m_pColliderManager = p_collider_manager;

    // Set default lifespans to 0
    for(int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
    {
        m_aStatusEffectsLifespans[i] = 0.0f;
    }

    // Set default lifespans to 0
    if(p_status_effects.size() > 0)
    {
        for(int i = 0; i < p_status_effects.size(); i++)
        {
            StatusComponent::StatusEffectType seType = p_status_effects.at(i).first;
            float lifespan = p_status_effects.at(i).second;

            if(seType != StatusComponent::StatusEffectType::NONE)
            {
                m_aStatusEffectsLifespans[seType] = lifespan;
            }
        }
    }
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
                    // Deal damage to the target
                    HealthComponent* thatHealth = thatObject.GetComponent<HealthComponent>();
                    if(thatHealth != nullptr)
                    {
                        thatHealth->Damage(m_fDamage);
                    }

                    // Apply status effects to the target
                    // StatusComponent* thatStatus = thatObject.GetComponent<StatusComponent>();
                    // if(thatStatus != nullptr && this->m_fSize > 0)
                    // {
                    //     for(auto info : this->m_StatusEffects)
                    //     {
                    //         StatusComponent::StatusEffectType seType = info.first;
                    //         float lifespan = info.second;

                    //         thatStatus->AddStatusEffect(seType, lifespan);
                    //     }
                    // }
                }
            }
        }
    }
}