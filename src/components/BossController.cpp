#include "BossController.h"
#include <W_GameObject.h>
#include <W_Transform2D.h>

#include <VelocityComponent.h>
#include <AnimatedSprite2D.h>
#include <HealthComponent.h>
#include <StatusComponent.h>
#include <ColliderComponent.h>
#include <PlayerController.h>
#include <HomingComponent.h>
#include <LabyrinthManager.h>

#include "../ColliderManager.h"

BossController::BossController()
{
}

BossController::~BossController()
{
}

void BossController::Init()
{
    // Initialize stats
    m_active = false;
    m_maxHealth = 1000;

    // Phase 1 stats
    m_throneBlockRange = 300;
    m_numSummons = 10;

    // Phase 2 stats
    m_axeAttackDamage = 80;
    m_axePunishDamage = 100;
    m_minDistToPlayer = 200;
    m_maxDistToPlayer = 600;

    // Phase 3 stats
    m_fireBreathDamage = 10; // Per projectile
    m_fireBreathRange = 400;
    m_chargeWindupTime = 1.0f; // Seconds
    m_chargeWindupTimer = 1.0f; // Seconds
    m_chargeWindupTint = glm::vec3(4.0f, 4.0f, 4.0f);
    m_chargeAttackDamage = 60;
    m_chargeAttackRange = 2000;
    m_stunTime = 1; // Seconds
    m_chargeTurningCapDegree = 9.0f; // Degrees
    m_chargeTurningDelay = 0.2f; // Seconds
    m_chargeSpeed = 480.0f;
    m_chargeKnockbackForce = 10000.0f;
    m_chargeChainCount = 3;

    m_searchSpeed = 160.0f;
    m_searchTimer = 5.0f;

    // Create components and cache pointers
    wolf::GameObject* pObject = GetGameObject();

    // Create transform
    pObject->DeleteComponent<wolf::Transform2D>();
    m_pTransform = &pObject->AddComponent<wolf::Transform2D>();
    m_pTransform->SetScale(glm::vec2(LabyrinthManager::SCALE));

    // Create velocity
    pObject->DeleteComponent<VelocityComponent>();
    m_pVelocity = &pObject->AddComponent<VelocityComponent>();

    // Create sprite
    pObject->DeleteComponent<AnimatedSprite2D>();
    m_pAnimSprite = &pObject->AddComponent<AnimatedSprite2D>("data/boss_anim_init.yaml");

    // Create health
    pObject->DeleteComponent<HealthComponent>();
    m_pHealth = &pObject->AddComponent<HealthComponent>(m_maxHealth);

    // Create status
    pObject->DeleteComponent<StatusComponent>();
    m_pStatus = &pObject->AddComponent<StatusComponent>();

    // Create collider
    pObject->DeleteComponent<ColliderComponent>();
    m_pCollider = &pObject->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, false, false);
    m_pCollider->AddColliderBox(glm::vec2(111, 156), glm::vec2(-52, 32));


    
    // Find player controller
    for (auto&&[_, controller] : pObject->GetScene().Each<PlayerController>())
    {
        m_pPlayerController = &controller;
        m_pPlayerObject = controller.GetGameObject();
        break;
    }
        // Create homing - Added by Nhật
    pObject->DeleteComponent<HomingComponent>();
    m_pHoming = &pObject->AddComponent<HomingComponent>(m_pPlayerObject, m_chargeTurningCapDegree, m_chargeTurningDelay, false);
    if (!m_pPlayerController)
    {
        wolf::Error("Boss controller init could not find player controller!");
    }

    EnterPhase1();
}

// <----------------- GENERAL UPDATE METHODS ----------------->

void BossController::Update(float delta)
{
    if (!m_active) return;

    // Call phase-specific update method
    switch (m_phase)
    {
        case FightPhase::PHASE_1:
            UpdatePhase1(delta);
            break;
        
        case FightPhase::PHASE_2:
            UpdatePhase2(delta);
            break;
        
        case FightPhase::PHASE_3:
            UpdatePhase3(delta);
            break;
    }

    UpdateAnimation();
}

void BossController::UpdateAnimation()
{
    // Set animation based on state, regardless of fight phase
}

// <----------------- PHASE 1 METHODS ----------------->

void BossController::EnterPhase1()
{
    m_phase = FightPhase::PHASE_1;
    m_state = State::SIT;

    // TODO: Phase 1 initialization logic
}

void BossController::UpdatePhase1(float delta)
{
    // TODO: Phase 1 update logic:
    // - Summon minitaurs periodically until limit reached
    // - Change to blocking animation when player attacks while close enough

    if (m_pHealth->GetHealth() < 2 * m_maxHealth / 3)
    {
        // TODO: Exit phase 1 logic
        EnterPhase2();
    }
}

void BossController::SummonMinitaur()
{

}

void BossController::BlockPlayerAttack()
{

}

// <----------------- PHASE 2 METHODS ----------------->

void BossController::EnterPhase2()
{
    m_phase = FightPhase::PHASE_2;
    m_state = State::APPROACH;

    // TODO: Phase 2 initialization logic
}

void BossController::UpdatePhase2(float delta)
{
    // TODO: Phase 2 update logic:
    // - Approach player and strafe when in neutral
    // - If player attacks and we are idle, attempt to dodge
    // - Periodically execute axe attack patterns

    if (m_pHealth->GetHealth() < m_maxHealth / 3)
    {
        // TODO: Exit phase 2 logic
        EnterPhase3();
    }
}

void BossController::StartAxeAttack()
{

}

void BossController::DodgePlayerAttack()
{

}

// <----------------- PHASE 3 METHODS ----------------->

void BossController::EnterPhase3()
{   
    m_phase = FightPhase::PHASE_3;
    m_state = State::SEARCHING;

    m_active = true;

    // TODO: Phase 3 initialization logic
}

void BossController::UpdatePhase3(float delta)
{
    // TODO: Phase 3 update logic:
    // - Stand in place and search for player when in neutral (can only see forward, rotate around?)
    // - When player found, if close, do fire breath attack
    // - if far away, do charge attack

    if (m_pHealth->GetHealth() <= 0 && m_state != State::DEAD)
    {
        m_state = State::DEAD;
        wolf::Log("It may have been the Minotaur's labyrinth but Theseus the GOAT");
    }

    switch (m_state)
    {
        case State::DEAD:
        {
            break;
        }

        case State::CHARGE_ATTACK:
        {
            AttackCharge(delta);
            break;
        }

        case State::FIRE_BREATH_ATTACK:
        {
            break;
        }

        case State::SEARCHING:
        {
            Search(delta);
            break;
        }

        case State::STUNNED:
        {
            Stunned(delta);
            break;
        }

        default:
        {
            break;
        }

        // TODO: Death animation + ending cutscene!
    }
}

void BossController::ChangeStatesPhase3(State p_state)
{
    // Return if state is not in phase 3
    if(p_state < State::SEARCHING)
    {
        return;
    }

    // End old state
    switch (m_state)
    {
        case State::CHARGE_ATTACK:
        {
            EndChargeAttack();
            break;
        }
        default:
        break;
    }

    // Start new state
    switch (p_state)
    {
        case State::CHARGE_ATTACK:
        {
            StartChargeAttack();
            break;
        }
        case State::FIRE_BREATH_ATTACK:
        {
            StartFireBreathAttack();
            break;
        }

        case State::STUNNED:
        {
            StartStunned();
            break;
        }

        default:
        break;
    }

    m_state = p_state;
}

void BossController::Search(float delta)
{
    if(m_searchTimer <= 0.0f)
    {
        glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
        float playerDistance = glm::distance(thisPos, playerPos);
        
        if(playerDistance <= m_chargeAttackRange)
        {
            ChangeStatesPhase3(State::CHARGE_ATTACK);
        }
        m_searchTimer = 5.0f;
    }
    else
    {
        m_searchTimer -= delta;

        MoveTowardsPlayer(delta);
    }

}

// Resued from GorgonController
void BossController::MoveTowardsPlayer(float delta)
{
    if (!m_pPlayerObject || !m_pVelocity || !m_pTransform) return;

    // Calculate the direction towards the player and move the Gorgon
        glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 

    // Calculate direction vector
    glm::vec2 direction = playerPos - thisPos;

    if (glm::length(direction) > 0.01f) {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_searchSpeed);

    } 
    else {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

void BossController::StartStunned()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));
}

void BossController::Stunned(float delta)
{
    if(m_stunTime <= 0.0f)
    {
        if(m_chargeChainCount > 0)
        {
            ChangeStatesPhase3(State::CHARGE_ATTACK);
        }
        else
        {
            ChangeStatesPhase3(State::SEARCHING);
        }

        m_stunTime = 1.0f;
        return;
    }
    else
    {
        m_stunTime -= delta;
    }
}

void BossController::StartFireBreathAttack()
{

}

void BossController::StartChargeAttack()
{
    m_chargeWindupTimer = m_chargeWindupTime;
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));
}

void BossController::AttackCharge(float delta)
{
    // If windup expired
    if(m_chargeWindupTimer <= 0.0f)
    {
        m_pVelocity->SetVelocity(glm::normalize(m_pVelocity->GetVelocity()) * m_chargeSpeed);

        // Get all colliders 
        for (auto&&[id, collider] : this->GetGameObject()->GetScene().Each<ColliderComponent>())
        {
            // If other collider is not the same collider, is active & is hitbox
            if (GetGameObject()->GetID() != id && collider.IsActive() && collider.IsHitbox())
            {
                // If colliders colliding
                if (ColliderManager::StaticMethodIsColliding(*m_pCollider, collider, delta))
                {
                    // If player, damge player
                    if(id == m_pPlayerObject->GetID())
                    {
                        glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                        glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
                        glm::vec2 playerDirection = (playerPos - thisPos) == glm::vec2(0.0f) ? 
                                                                                                glm::vec2(1.0f, 0.0f):
                                                                                                glm::normalize(playerPos - thisPos);
                        m_pPlayerObject->GetComponent<HealthComponent>()->Damage(100.0f);
                        m_pPlayerObject->GetComponent<VelocityComponent>()->ApplyKnockback(playerDirection, m_chargeKnockbackForce);
                    }

                    // Change to STUNNED state
                    ChangeStatesPhase3(State::STUNNED);
                    return;
                }
            }
        }
    }
    else
    {
        // Update windup timer
        m_chargeWindupTimer -= delta;

        // If entering attack
        if(m_chargeWindupTimer <= 0.0f)
        {
            m_pAnimSprite->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));

            // Calculate celocity
            glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
            glm::vec2 velo = glm::normalize(playerPos - thisPos) * m_chargeSpeed;
            
            // Set components for charge attack
            m_pVelocity->SetVelocity(velo);
            m_pHoming->SetActive(true);
            
            // Reset chain count
            if(m_chargeChainCount <= 0)
            {
                m_chargeChainCount = 3;
            }
        }
        // If still windup
        else
        {
            glm::vec3 currentTint = m_pAnimSprite->GetTint();
            glm::vec3 nextTint = currentTint + glm::vec3(delta / (m_chargeWindupTime * 0.5f));
            m_pAnimSprite->SetTint(nextTint);
        }

    }
    
}

void BossController::EndChargeAttack()
{
    m_pHoming->SetActive(false);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));

    if(m_chargeChainCount > 0)
    {
        m_chargeChainCount--;
    }
}