#include "EnemyController.h"
#include <iostream>

// Constructor to initialize the chase speed and set state to IDLE
EnemyController::EnemyController(float chaseSpeed)
    : m_chaseSpeed(chaseSpeed), m_state(EnemyState::IDLE)
{
}

void EnemyController::Init()
{
    auto* pGameObject = GetGameObject();
    if (pGameObject)
    {
        // Attempt to get necessary components
        m_pTransform = pGameObject->GetComponent<wolf::Transform2D>();
        m_pVelocity = pGameObject->GetComponent<VelocityComponent>();
        m_pHealth = pGameObject->GetComponent<HealthComponent>();
        // m_pAnim = pGameObject->GetComponent<AnimatedSprite2D>();

        // Log component status
        std::cout << "EnemyController Init: " 
                  << "Transform: " << (m_pTransform ? "Initialized" : "Not Initialized") << ", "
                  << "Velocity: " << (m_pVelocity ? "Initialized" : "Not Initialized") << ", "
                  << "Health: " << (m_pHealth ? "Initialized" : "Not Initialized") << ", " << std::endl;

        // If any component is missing, print a log and return
        if (!m_pTransform || !m_pVelocity || !m_pHealth)
        {
            std::cerr << "Error: Components not properly initialized in EnemyController!" << std::endl;
            return;
        }

        // Search for the player object in the scene and set it as the target
        for (auto&& [entity, playerController] : pGameObject->GetScene().Each<PlayerController>())
        {
            m_pTarget = playerController.GetGameObject();
            std::cout << "Player found and set as target!" << std::endl;
            break; // Assume there's only one player in the scene
        }

        if (!m_pTarget)
        {
            std::cerr << "Error: No player found in the scene!" << std::endl;
        }
    }
}

void EnemyController::Update(float delta)
{
    // Ensure components and target are initialized
    if (!m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget) {
        std::cout << "Components or target not initialized." << std::endl;
        return;
    }

    // std::cout << "Current State: " << static_cast<int>(m_state) << std::endl;

    // State handling
    switch (m_state)
    {
        case EnemyState::IDLE:
            HandleIdleState(delta);
            break;
        case EnemyState::CHASING:
            HandleChasingState(delta);
            break;
        case EnemyState::ATTACKING:
            HandleAttackingState(delta);
            break;
        case EnemyState::DEATH:
            HandleDeathState();
            break;
    }
}

void EnemyController::HandleIdleState(float delta)
{
    // std::cout << "Enemy is in IDLE state." << std::endl;

    if (IsPlayerInRange())
    {
        // std::cout << "Player detected in range. Transitioning to CHASING state." << std::endl;
        m_state = EnemyState::CHASING;
    }

    // Play idle animation, if available
    // if (m_pAnim)
    // {
    //     std::cout << "Playing Idle animation." << std::endl;
    //     m_pAnim->SetAnimation("Idle");
    // }
}

void EnemyController::HandleChasingState(float delta) 
{
    if (!m_pTarget || m_state == EnemyState::DEATH) return;

    // Calculate distance to player
    float distance = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());
    
    // Check if the player is within melee range to start attacking
    if (distance <= m_meleeRange) 
    {
        // std::cout << "Player is within melee range. Transitioning to ATTACKING state." << std::endl;
        m_state = EnemyState::ATTACKING;
        return;
    }

    MoveTowardsTarget(delta);

    // Transition back to idle if player is out of detection range
    if (!IsPlayerInRange()) 
    {
        // std::cout << "Player out of range. Transitioning to IDLE state." << std::endl;
        m_state = EnemyState::IDLE;
    }

    // Update animation to chasing animation (optional)
    // if (m_pAnim)
    // {
    //     std::cout << "Playing Walk animation." << std::endl;
    //     m_pAnim->SetAnimation("Walk");
    // }
}

void EnemyController::HandleAttackingState(float delta) 
{
    if (!m_pTarget || m_state == EnemyState::DEATH) return;

    // Calculate distance to player
    float distance = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());
    
    // Check if we are still within melee range
    if (distance > m_meleeRange) 
    {
        // std::cout << "Player out of melee range. Transitioning to CHASING state." << std::endl;
        m_state = EnemyState::CHASING;
        return;
    }

    // Apply attack cooldown timer
    m_attackTimer -= delta;
    if (m_attackTimer <= 0.0f) 
    {
        // Perform attack and reset the timer
        // std::cout << "Enemy is attacking the player!" << std::endl;
        ApplyDamageToPlayer();

        // Reset attack timer to cooldown duration
        m_attackTimer = m_attackCooldown;

        // After attacking, check if we should go back to chasing or continue attacking
        m_state = (distance > m_meleeRange) ? EnemyState::CHASING : EnemyState::ATTACKING;
    }

    // Play attacking animation, if available
    // if (m_pAnim)
    // {
    //     std::cout << "Playing Attack animation." << std::endl;
    //     m_pAnim->SetAnimation("Attack");
    // }
}

void EnemyController::HandleDeathState()
{
    // std::cout << "Enemy is in DEATH state. Destroying object." << std::endl;

    // Destroy the game object
    if (GetGameObject())
    {
        GetGameObject()->Delete();
    }
}

void EnemyController::MoveTowardsTarget(float delta)
{
    if (!m_pTransform || !m_pTarget || !m_pVelocity) return;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Debug log positions
    // std::cout << "Enemy Current Position: (" << currentPosition.x << ", " << currentPosition.y << ")"
    //           << ", Target Position: (" << targetPosition.x << ", " << targetPosition.y << ")" << std::endl;

    if (std::isnan(currentPosition.x) || std::isnan(currentPosition.y) || 
        std::isnan(targetPosition.x) || std::isnan(targetPosition.y))
    {
        std::cerr << "Error: NaN values detected in position!" << std::endl;
        return;
    }

    glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
    m_pVelocity->SetVelocity(direction * m_chaseSpeed);

    // std::cout << "Minitaur Velocity: (" << m_pVelocity->GetVelocity().x << ", " << m_pVelocity->GetVelocity().y << ")" << std::endl;
}

bool EnemyController::IsPlayerInRange() const
{
    if (!m_pTransform || !m_pTarget) return false;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Calculate distance to the player
    float distance = glm::length(targetPosition - currentPosition);

    // std::cout << "Distance to player: " << distance << std::endl; // Log distance to player

    // Define detection range (can be a configurable variable)
    const float detectionRange = 300.0f; // Example value

    return distance <= detectionRange;
}

void EnemyController::ApplyDamageToPlayer() 
{
    // Ensure target player exists and has a health component
    if (m_pTarget) 
    {
        auto* playerHealth = m_pTarget->GetComponent<HealthComponent>();
        if (playerHealth) 
        {
            // Apply damage to the player's health
            playerHealth->Damage(m_baseDamage);
            std::cout << "Applied " << m_baseDamage << " damage to the player!" << std::endl;
        }
    }
}
