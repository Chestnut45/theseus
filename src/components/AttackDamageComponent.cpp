//-----------------------------------------------------------------------------
// File: AttackDamageComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Modifications : Youssef Ashraf
// Checks for collision with HURTBOXDRs if owner object has HURTBOXDD.
//-----------------------------------------------------------------------------

#include "AttackDamageComponent.h"
#include "ColliderComponent.h"
#include "TimedDestroyerComponent.h"
#include "EnemyController.h"
#include "GorgonController.h"
#include "HarpyController.h"
#include "MinitaurController.h"
#include "NPCComponent.h"
#include "InfightingEvent.h"
#include "ThrowableObjectComponent.h"


AttackDamageComponent::AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, float knockbackMagnitude, std::vector<std::pair<StatusComponent::StatusEffectType, float>> p_status_effects, wolf::GameObject* owner)
{
    this->m_fDamage = p_damage;
    this->m_pColliderManager = p_collider_manager;
    this->m_knockbackMagnitude = knockbackMagnitude;
    this->m_pOwner = owner;

    // Set default lifespans to 0
    for(int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
    {
        m_aStatusEffectsLifespans[i] = 0.0f;
    }

    // Set lifespans
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
    wolf::Transform2D* thisTransform = thisObject->GetComponent<wolf::Transform2D>();


    // If collider of this object is HurtboxDD
    if (thisCollider != nullptr && thisCollider->IsHurtboxDamageDealer())
    {
        for (auto&&[thatID, thatHealth, thatCollider] : this->GetGameObject()->GetScene().Each<HealthComponent, ColliderComponent>())
        {
            // If collider of that object is HurtboxDR
            if (thatCollider.IsActive() && thatCollider.IsHurtboxDamageReceiver())
            {
                // If colliders colliding
                if (this->m_pColliderManager->IsColliding(*thisCollider, thatCollider, p_dt))
                {
                    wolf::GameObject* thatObject = thatHealth.GetGameObject();
                    // Get the owner of the projectile
                    if (m_pOwner)
                    {
                        if (m_pOwner->HasAny<GorgonController, HarpyController>() &&
                            thatObject->HasAny<GorgonController, HarpyController, MinitaurController>())
                        {
                            // wolf::Log("Infighting triggered: " + std::to_string(m_pOwner->GetID()) + 
                            //           " hit " + std::to_string(thatObject->GetID()));
                            wolf::EventManager::TriggerEvent(InfightingEvent(m_pOwner, thatObject));
                        }
                    }
                    // Deal damage
                    thatHealth.Damage(m_fDamage);
                    
                    if (thisObject->HasAll<ThrowableObjectComponent>())
                    {
                        wolf::Audio::Play("data/sounds/sfx_throwable_break.wav", 0.32f);
                    }

                    // Apply status effects to the target
                    StatusComponent* thatStatus = thatObject->GetComponent<StatusComponent>();
                    if(thatStatus != nullptr)
                    {
                        for(int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
                        {
                            float lifespan = this->m_aStatusEffectsLifespans[i];
                            if(lifespan != 0.0f)
                            {
                                StatusComponent::StatusEffectType seType = static_cast<StatusComponent::StatusEffectType>(i);

                                // Play burn sfx whenever a burn status effect is applied from a projectile
                                if (seType == StatusComponent::StatusEffectType::BURNING)
                                {
                                    // Ensure falloff for potentially stacked sounds
                                    wolf::Audio::Play("data/sounds/sfx_fireball_extinguish.wav", 0.64f, 0.0f, 0.0f, true);
                                }

                                thatStatus->AddStatusEffect(seType, lifespan);
                            }

                        }
                    }

                    // Apply knockback if magnitude > 0
                    if (m_knockbackMagnitude > 0.0f)
                    {
                        auto* thatTransform = thatObject->GetComponent<wolf::Transform2D>();
                        auto* velocityComponent = thatObject->GetComponent<VelocityComponent>();
                        if (thatTransform && velocityComponent)
                        {
                            glm::vec2 knockbackDirection = glm::normalize(
                                thatTransform->GetGlobalPosition() - thisTransform->GetGlobalPosition()
                            );
                            velocityComponent->ApplyKnockback(knockbackDirection, m_knockbackMagnitude);
                        }
                    }
                    
                    // Deactivate collider and add a timed destroyer component to the object
                    // NOTE: A delayed destruction is used to ensure AOE attacks can affect all targets
                    // in a single frame instead of immediately deleting the object on first contact.
                    // Maybe this should be configurable in the future as a DamageType enum or similar?
                    thisCollider->SetActive(false);

                    // If another destroyer exists, this one should take precedence since it's for 0 frames
                    thisObject->DeleteComponent<TimedDestroyerComponent>();
                    thisObject->AddComponent<TimedDestroyerComponent>(0, true);

                    // Stun target if it has any of the following controllers
                    if(thatObject->HasAny<GorgonController, HarpyController, MinitaurController, NPCComponent>())
                    {
                        // Stun gorgon
                        GorgonController* gorgonController = thatObject->GetComponent<GorgonController>();
                        if(gorgonController != nullptr)
                        {
                            gorgonController->ChangeState(EnemyController::EnemyState::STUNNED);
                        }

                        // Stun Harpy
                        HarpyController* harpyController = thatObject->GetComponent<HarpyController>();
                        if(harpyController != nullptr)
                        {
                            harpyController->ChangeState(EnemyController::EnemyState::STUNNED);
                        }

                        // Stun Minitaur
                        MinitaurController* minitaurController = thatObject->GetComponent<MinitaurController>();
                        if(minitaurController != nullptr)
                        {
                            minitaurController->ChangeState(EnemyController::EnemyState::STUNNED);
                        }

                        // Stun NPC
                        NPCComponent* npcComp = thatObject->GetComponent<NPCComponent>();
                        if(npcComp != nullptr)
                        {
                            npcComp->StunNPC();
                        }
                    }
                }
            }
        }
    }
}