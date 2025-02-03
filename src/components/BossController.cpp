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
#include <HomingComponent.h>
#include <W_Timer.h>

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
    m_maxHealth = 6500;

    // Phase 1 stats
    m_throneBlockRange = 300;
    m_numSummons = 10;

    // Phase 2 stats
    m_axeAttackDamage = 125;
    m_axePunishDamage = 100;
    m_minDistToPlayer = 160;
    m_maxDistToPlayer = 280;
    m_strafeClockwise = true;
    m_axeSummoned = false;
    m_strafeSpeed = 160.0f;
    m_chaseSpeed = 240.0f;

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

    EnterPhase2();
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
    m_axeAttackTimer.Restart();
}

void BossController::UpdatePhase2(float delta)
{
    // Timing variables
    static wolf::RNG rng;
    static float nextStrafeSwap = 1.0f;
    static float nextAttackTime = 1.0f;

    // Query player spatial info
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 toPlayer = playerPos - m_pTransform->GetGlobalPosition();
    float distToPlayer = glm::length(toPlayer);
    glm::vec2 dirToPlayer = glm::normalize(toPlayer);

    // Get rotated direction
    glm::mat4 rotation = glm::rotate(glm::radians(m_strafeClockwise ? 90.0f : -90.0f), glm::vec3(0, 0, 1));
    glm::vec2 rotated = glm::vec2(rotation * glm::vec4(dirToPlayer.x, dirToPlayer.y, 0, 1));

    // Query player controller state
    bool playerAttacking = m_pPlayerController->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING;

    // Update axe if it exists
    if (m_pAxeCollider)
    {
        // Play whoosh wfx
        if (m_whooshTimer.Elapsed() >= 0.32)
        {
            wolf::Audio::Play("data/sounds/sfx_axe_whoosh.wav", 0.4f);
            m_whooshTimer.Restart();
        }

        // Calculate vector from axe to player
        glm::vec2 axePos = m_pAxeCollider->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 axeToPlayer = glm::normalize(playerPos - axePos);
        
        // Check for player collision
        auto* pPlayerCollider = m_pPlayerObject->GetComponent<ColliderComponent>();
        if (pPlayerCollider->IsHurtbox() && ColliderManager::StaticMethodIsColliding(*m_pAxeCollider, *pPlayerCollider, delta))
        {
            auto* pPlayerHealth = m_pPlayerObject->GetComponent<HealthComponent>();
            auto* pPlayerVel = m_pPlayerObject->GetComponent<VelocityComponent>();
            if (pPlayerHealth && pPlayerVel)
            {
                pPlayerHealth->Damage(m_axeAttackDamage);
                pPlayerVel->ApplyKnockback(axeToPlayer, 4500.0f);
            }
        }

        // Collect axe and return to state
        if (m_axeAttackTimer.Elapsed() > 1.0f && glm::distance(axePos, m_pTransform->GetGlobalPosition()) < 128.0f)
        {
            m_pAxeCollider->GetGameObject()->Delete();
            m_pAxeCollider = nullptr;
            m_state = State::APPROACH;
            m_axeAttackTimer.Restart();
            m_axeSummoned = false;
            m_pAnimSprite->SetTint(glm::vec3(1.0f));
            m_whooshTimer.Reset();
        }
    }

    // Update based on state
    switch (m_state)
    {
        case State::APPROACH:

            // Dodge player attack
            if (playerAttacking && (distToPlayer < m_maxDistToPlayer || m_pPlayerController->GetHeldWeapon()->GetWeaponType() == WeaponType::BOW))
            {
                DodgePlayerAttack(dirToPlayer);
                break;
            }

            // Start axe attack
            if (m_axeAttackTimer.Elapsed() > nextAttackTime)
            {
                StartAxeAttack();
                nextAttackTime = rng.NextFloat(2.0f, 6.0f);
                break;
            }

            // Normal approach logic
            if (distToPlayer > m_maxDistToPlayer)
            {
                // Approach player
                m_pVelocity->SetVelocity(dirToPlayer * m_chaseSpeed);
            }
            else if (distToPlayer < m_minDistToPlayer)
            {
                // Back away from player
                m_pVelocity->SetVelocity(-dirToPlayer * m_chaseSpeed);
            }
            else
            {
                // Switch to strafe state if within proper range
                m_state = State::STRAFE;
                m_strafeSwapTimer.Restart();
            }

            break;
        
        case State::STRAFE:
        {
            // Randomly change strafe direction
            if (!m_strafeSwapTimer.IsRunning())
            {
                m_strafeSwapTimer.Restart();
                nextStrafeSwap = rng.NextFloat(0.5f, 5.0f);
            }
            if (m_strafeSwapTimer.Elapsed() >= nextStrafeSwap)
            {
                m_strafeClockwise = !m_strafeClockwise;
                m_strafeSwapTimer.Reset();
            }
            
            // Move along strafe direction
            m_pVelocity->SetVelocity(rotated * m_strafeSpeed);

            // Switch back to approach if necessary
            if (distToPlayer > m_maxDistToPlayer || distToPlayer < m_minDistToPlayer)
            {
                m_state = State::APPROACH;
                break;
            }

            // Dodge player attack
            if (playerAttacking)
            {
                DodgePlayerAttack(dirToPlayer);
                break;
            }

            // Start axe attack
            if (m_axeAttackTimer.Elapsed() > nextAttackTime)
            {
                StartAxeAttack();
                nextAttackTime = rng.NextFloat(2.0f, 6.0f);
                break;
            }

            break;
        }

        case State::DODGE:

            // Return to approach state
            if (m_dodgeTimer.Elapsed() > 0.6f)
            {
                m_state = State::APPROACH;
                m_dodgeTimer.Reset();
                m_pAnimSprite->SetTint(glm::vec3(1.0f));
            }

            break;
        
        case State::AXE_ATTACK:

            if (!m_axeSummoned)
            {
                // Update tint for tell
                m_pAnimSprite->SetTint(glm::vec3(1.0f) * (float)(m_axeAttackTimer.Elapsed() + 1.0f));

                // Spawn projectile
                if (m_axeAttackTimer.Elapsed() > 1.0f)
                {
                    // Update timer and flag
                    m_axeAttackTimer.Restart();
                    m_axeSummoned = true;
                    m_pAnimSprite->SetTint(glm::vec3(1.0f));

                    // Create the axe object
                    auto& axe = GetGameObject()->GetScene().CreateObject2D();
                    auto& transform = *axe.GetComponent<wolf::Transform2D>();
                    transform.SetPosition(m_pTransform->GetGlobalPosition());
                    transform.SetScale(glm::vec2(3.0f));
                    auto& velocity = axe.AddComponent<VelocityComponent>();
                    velocity.SetVelocity(dirToPlayer * 640.0f);
                    auto& sprite = axe.AddComponent<AnimatedSprite2D>("data/axe_spin_anim_init.yaml");
                    sprite.SetOriginToCenterOfFrame();
                    auto& homing = axe.AddComponent<HomingComponent>(GetGameObject(), 8.0f, 0.1f);
                    m_pAxeCollider = &axe.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, false, false);
                    m_pAxeCollider->AddColliderBox(glm::vec2(224), glm::vec2(-112, 112));

                    // Change state
                    m_state = State::APPROACH;

                    wolf::Audio::Play("data/sounds/sfx_axe_whoosh.wav", 0.4f);
                    m_whooshTimer.Restart();
                }
            }
            else
            {
                // We shouldn't reach here anyway, but safety!
                m_state = State::APPROACH;
            }

            break;
    }

    // Under half health, change to phase 3
    if (m_pHealth->GetHealth() <= m_maxHealth / 2)
    {
        // Cleanup
        if (m_pAxeCollider)
        {
            m_pAxeCollider->GetGameObject()->Delete();
            m_pAxeCollider = nullptr;
        }

        m_pAnimSprite->SetTint(glm::vec3(1.0f));

        EnterPhase3();
    }
}

void BossController::StartAxeAttack()
{
    // Can't spawn multiple axes!
    if (m_pAxeCollider) return;

    // Change state
    m_state = State::AXE_ATTACK;
    m_axeAttackTimer.Restart();
    m_pVelocity->SetVelocity(glm::vec2(0.0f));

    wolf::Audio::Play("data/sounds/sfx_boss_growl.wav", 1.1f);
}

void BossController::DodgePlayerAttack(const glm::vec2& dirToPlayer)
{
    // Change state
    m_state = State::DODGE;
    m_dodgeTimer.Restart();
    m_pAnimSprite->SetTint(glm::vec3(1.0f, 1.0f, 0.0f));

    // Get randomly rotated direction
    static wolf::RNG rng;
    glm::mat4 rotation = glm::rotate(glm::radians(rng.FlipCoin() ? 90.0f : -90.0f), glm::vec3(0, 0, 1));
    glm::vec2 rotated = glm::vec2(rotation * glm::vec4(dirToPlayer.x, dirToPlayer.y, 0, 1));

    // Dodge away from player for melee, dodge sideways for the bow
    WeaponItem* pWeapon = m_pPlayerController->GetHeldWeapon();
    m_dodgeDir = (pWeapon && pWeapon->GetWeaponType() == WeaponType::BOW) ? -rotated : -dirToPlayer;
    m_pVelocity->SetVelocity(m_dodgeDir * m_chaseSpeed * 1.4f);
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