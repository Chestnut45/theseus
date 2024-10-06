#include "EnemyController.h"
#include <iostream>

EnemyController::EnemyController(float chaseSpeed)
    : m_chaseSpeed(chaseSpeed), m_state(EnemyState::IDLE)
{
}

void EnemyController::Init()
{
    auto* pGameObject = GetGameObject();
    if (pGameObject)
    {
        // Initialize and validate components
        m_pTransform = pGameObject->GetComponent<wolf::Transform2D>();
        m_pVelocity = pGameObject->GetComponent<VelocityComponent>();
        m_pHealth = pGameObject->GetComponent<HealthComponent>();

        // Check if the essential components are initialized properly
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

    // State handling
    switch (m_state)
    {
        case EnemyState::IDLE:
            HandleIdleState();
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

void EnemyController::HandleIdleState()
{
    if (IsPlayerInRange())
    {
        m_state = EnemyState::CHASING;
    }
}

void EnemyController::HandleChasingState(float delta)
{
    if (!m_pTarget || m_state == EnemyState::DEATH) return;

    // Calculate distance to player
    float distance = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());
    
    // Check if the player is within melee range to start attacking
    if (distance <= m_meleeRange) 
    {
        m_state = EnemyState::ATTACKING;
        return;
    }

    MoveTowardsTarget(delta);

    // Transition back to idle if player is out of detection range
    if (!IsPlayerInRange()) 
    {
        m_state = EnemyState::IDLE;
    }
}

void EnemyController::HandleAttackingState(float delta)
{
    if (!m_pTarget || m_state == EnemyState::DEATH) return;

    // Calculate distance to player
    float distance = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    // If the player moves out of melee range, go back to chasing
    if (distance > m_meleeRange)
    {
        m_state = EnemyState::CHASING;
        return;
    }

    // Attack cooldown timer
    m_attackTimer -= delta;
    if (m_attackTimer <= 0.0f)
    {
        ApplyDamageToPlayer(); // Deal damage to the player
        m_attackTimer = m_attackCooldown; // Reset the timer
    }
}

void EnemyController::HandleDeathState()
{
    // Handle enemy death state, remove the enemy from the scene
    if (GetGameObject())
    {
        GetGameObject()->Delete(); // This should safely remove the enemy from the scene
    }
}

void EnemyController::MoveTowardsTarget(float delta)
{
    if (!m_pTransform || !m_pTarget || !m_pVelocity) return;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    if (std::isnan(currentPosition.x) || std::isnan(currentPosition.y) || 
        std::isnan(targetPosition.x) || std::isnan(targetPosition.y))
    {
        std::cerr << "Error: NaN values detected in position!" << std::endl;
        return;
    }

    glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
    m_pVelocity->SetVelocity(direction * m_chaseSpeed);
}

bool EnemyController::IsPlayerInRange() const
{
    if (!m_pTransform || !m_pTarget) return false;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Calculate distance to the player
    float distance = glm::length(targetPosition - currentPosition);

    // Define detection range
    const float detectionRange = 300.0f; // Example value

    return distance <= detectionRange;
}

void EnemyController::ApplyDamageToPlayer()
{
    // Ensure the target player exists and has a health component
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