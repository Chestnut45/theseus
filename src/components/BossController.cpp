#include "BossController.h"

#include <W_GameObject.h>
#include <W_Transform2D.h>

#include <VelocityComponent.h>
#include <AnimatedSprite2D.h>
#include <HealthComponent.h>
#include <StatusComponent.h>
#include <ColliderComponent.h>
#include <PlayerController.h>
#include <LabyrinthManager.h>
#include <HomingComponent.h>
#include <W_Timer.h>

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
    m_axeAttackDamage = 80;
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
    m_chargeAttackDamage = 60;
    m_chargeAttackRange = 2000;
    m_stunTime = 4; // Seconds

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

    // TODO: Phase 2 initialization logic
}

void BossController::UpdatePhase2(float delta)
{
    // TODO: Phase 2 update logic:
    // - Approach player and strafe when in neutral
    // - If player attacks and we are idle, attempt to dodge
    // - Periodically execute axe attack patterns

    // Timing variables
    static wolf::RNG rng;
    static float nextStrafeSwap = 1.0f;

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

    // Update based on state
    switch (m_state)
    {
        case State::APPROACH:

            // Dodge player attack
            if (playerAttacking && distToPlayer < m_maxDistToPlayer)
            {
                DodgePlayerAttack(dirToPlayer);
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
            if (!m_axeAttackTimer.IsRunning() || m_axeAttackTimer.Elapsed() > 2.0f)
            {
                StartAxeAttack();
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
            }

            break;
        
        case State::AXE_ATTACK:

            if (!m_axeSummoned)
            {
                // Update tint for tell
                m_pAnimSprite->SetTint(glm::vec3(1.0f) * (float)m_axeAttackTimer.Elapsed() * 2.0f);

                // Spawn projectile
                if (m_axeAttackTimer.Elapsed() > 2.0f)
                {
                    // Update timer and flag
                    m_axeAttackTimer.Reset();
                    m_axeSummoned = true;
                    m_pAnimSprite->SetTint(glm::vec3(1.0f));

                    // Create the axe object
                    auto& axe = GetGameObject()->GetScene().CreateObject2D();
                    auto& transform = *axe.GetComponent<wolf::Transform2D>();
                    transform.SetPosition(m_pTransform->GetGlobalPosition());
                    transform.SetScale(glm::vec2(3.0f));
                    auto& velocity = axe.AddComponent<VelocityComponent>();
                    velocity.SetVelocity(dirToPlayer * 450.0f);
                    auto& sprite = axe.AddComponent<AnimatedSprite2D>("data/axe_spin_anim_init.yaml");
                    sprite.SetOriginToCenterOfFrame();
                    auto& homing = axe.AddComponent<HomingComponent>(GetGameObject(), 8.0f, 0.1f);
                    m_pAxeCollider = &axe.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, false, false, GetGameObject()->GetID());
                    m_pAxeCollider->AddColliderBox(glm::vec2(224), glm::vec2(-112, 112));
                }
            }
            else
            {
                // Axe has already been summoned, wait for return
                
            }

            break;
    }

    // Under half health, change to phase 3
    if (m_pHealth->GetHealth() <= m_maxHealth / 2)
    {
        EnterPhase3();
    }
}

void BossController::StartAxeAttack()
{
    // Change state
    m_state = State::AXE_ATTACK;
    m_axeAttackTimer.Restart();
    
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void BossController::DodgePlayerAttack(const glm::vec2& dirToPlayer)
{
    // Change state
    m_state = State::DODGE;
    m_dodgeTimer.Restart();

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

        // TODO: Death animation + ending cutscene!
    }
}

void BossController::StartFireBreathAttack()
{

}

void BossController::StartChargeAttack()
{

}