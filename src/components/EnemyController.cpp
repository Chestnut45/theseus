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

        // Log component status
        std::cout << "EnemyController Init: " 
                  << "Transform: " << (m_pTransform ? "Initialized" : "Not Initialized") << ", "
                  << "Velocity: " << (m_pVelocity ? "Initialized" : "Not Initialized") << ", "
                  << "Health: " << (m_pHealth ? "Initialized" : "Not Initialized") << std::endl;

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

    std::cout << "Current State: " << static_cast<int>(m_state) << std::endl;

    // State handling
    switch (m_state)
    {
        case EnemyState::IDLE:
            HandleIdleState(delta);
            break;
        case EnemyState::CHASING:
            HandleChasingState(delta);
            break;
        case EnemyState::DEATH:
            HandleDeathState();
            break;
    }
}

void EnemyController::HandleIdleState(float delta)
{
    std::cout << "Enemy is in IDLE state." << std::endl;

    if (IsPlayerInRange())
    {
        std::cout << "Player detected in range. Transitioning to CHASING state." << std::endl;
        m_state = EnemyState::CHASING;
    }

    // Play idle animation, if available
    if (m_pAnim)
    {
        m_pAnim->SetAnimation("Idle");
    }
}

void EnemyController::HandleChasingState(float delta)
{
    std::cout << "Enemy is in CHASING state." << std::endl;

    if (!m_pTarget || m_state == EnemyState::DEATH) return;

    MoveTowardsTarget(delta);

    // Transition back to idle if player is out of range
    if (!IsPlayerInRange())
    {
        std::cout << "Player out of range. Transitioning to IDLE state." << std::endl;
        m_state = EnemyState::IDLE;
        return;
    }

}
void EnemyController::HandleDeathState()
{
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
    std::cout << "Enemy Current Position: (" << currentPosition.x << ", " << currentPosition.y << ")"
              << ", Target Position: (" << targetPosition.x << ", " << targetPosition.y << ")" << std::endl;

    if (std::isnan(currentPosition.x) || std::isnan(currentPosition.y) || 
        std::isnan(targetPosition.x) || std::isnan(targetPosition.y))
    {
        std::cerr << "Error: NaN values detected in position!" << std::endl;
        return;
    }

    glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
    m_pVelocity->SetVelocity(direction * m_chaseSpeed);

    std::cout << "Minitaur Velocity: (" << m_pVelocity->GetVelocity().x << ", " << m_pVelocity->GetVelocity().y << ")" << std::endl;
}

bool EnemyController::IsPlayerInRange() const
{
    if (!m_pTransform || !m_pTarget) return false;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Calculate distance to the player
    float distance = glm::length(targetPosition - currentPosition);

    std::cout << "Distance to player: " << distance << std::endl; // Log distance to player

    // Define detection range (can be a configurable variable)
    const float detectionRange = 300.0f; // Example value

    return distance <= detectionRange;
}
