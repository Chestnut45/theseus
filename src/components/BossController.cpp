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

#include "../DDACalculator.h"
#include "GLShapesRenderer.h"
#include "../TileFireManager.h"

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
    m_fireBreathRange = 300;
    m_fireAttackDuration = 20.0f;
    m_turningCapRadian = 5.0f * (M_PI / 180.0f); // Maximum angle for each turn instance
    m_turningDelay = 0.2f;      // Delay between each turn instance
    m_turningTimer = 0.0f;
    m_lastDirection = glm::vec2(1.0f, 0.0f);
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

    EnterPhase3();
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
    ChangeStatesPhase3(State::FIRE_BREATH_ATTACK);

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

    switch (m_state)
    {
        case State::DEAD:
        {
            break;
        }

        case State::FIRE_BREATH_ATTACK:
        {
            AttackFireBreath(delta);
            break;
        }
        default:
        break;

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
        default:
        break;
    }

    // Start new state
    switch (p_state)
    {
        case State::FIRE_BREATH_ATTACK:
        StartFireBreathAttack();

        default:
        break;
    }

    m_state = p_state;
}

void BossController::StartChargeAttack()
{

}

void BossController::StartFireBreathAttack()
{
    m_fireAttackDuration = 250.0f;
    GetGameObject()->GetComponent<VelocityComponent>()->SetVelocity(glm::vec2(0.0f, 0.0f));
    glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    m_lastDirection = glm::normalize(playerPos - thisPos);
}

void BossController::AttackFireBreath(float delta)
{
    // Update attack timer & state
    if(m_fireAttackDuration <= 0.0f)
    {
        ChangeStatesPhase3(State::SEARCHING);
        return;    
    }
    m_fireAttackDuration -= delta;

    // Get data
    glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 endPos = thisPos + m_lastDirection * m_fireBreathRange;

    // Add fire range indicator
    GLShapesRenderer::GetInstance()->AddLine({thisPos.x, thisPos.y, 1, 0, 0 , 1}, {endPos.x, endPos.y, 1, 0, 0 , 1});
    
    // If turning delay expired
    if(this->m_turningTimer >= this->m_turningDelay)
    {
        // Turn towards player
        TurnToPlayer(delta);
        
        // Get all tiles burnt
        std::vector<glm::ivec2> tiles = DDACalculator::GetInstance()->GetTraversedTiles(thisPos, endPos, true);

        // Add fire tiles
        for (glm::ivec2 tile : tiles)
        {
            TileFireManager::GetInstance()->AddFireTile(tile, 10.0f);
        }
    }
    else
    {
        this->m_turningTimer += delta;
    }
}

void BossController::TurnToPlayer(float delta)
{
    // Reset timer
    this->m_turningTimer = 0.0f;

    // Get data
    glm::vec2 thisPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 playerPos = m_pPlayerObject->GetComponent<wolf::Transform2D>()->GetGlobalPosition(); 
    float playerDistance = glm::length(thisPos - playerPos);
    
    if(playerDistance > 0.0f)
    {
        glm::vec2 playerDirection = glm::normalize(playerPos - thisPos);   
        
        // Calculate angle 
        float dotProduct = glm::dot(m_lastDirection, playerDirection);
        float cos = glm::clamp(dotProduct, -1.0f, 1.0f);
        float radAngle = glm::acos(cos);

        // If turning angle is smaller than cap
        if(std::abs(radAngle) <= std::abs(m_turningCapRadian))
        {
            m_lastDirection = playerDirection;
        }

        else
        {
            // Calculate rotation side
            float side = glm::cross(glm::vec3(m_lastDirection.x, m_lastDirection.y, 0), glm::vec3(playerDirection.x, playerDirection.y, 0)).z;
            glm::vec2 newDirection = glm::vec2(0.0f, 0.0f);

            float capSin = glm::sin(m_turningCapRadian);
            float capCos = glm::cos(m_turningCapRadian);
            // Left
            if(side >= 0.0f)
            {
                newDirection.x = m_lastDirection.x * capCos - m_lastDirection.y * capSin;
                newDirection.y = m_lastDirection.y * capCos + m_lastDirection.x * capSin;
            }
            // Right
            else
            {
                newDirection.x = m_lastDirection.x * capCos + m_lastDirection.y * capSin;
                newDirection.y = m_lastDirection.y * capCos - m_lastDirection.x * capSin;
            }

            m_lastDirection = newDirection;
        }
    }
}