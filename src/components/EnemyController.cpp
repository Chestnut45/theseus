#include "EnemyController.h"

// Constructor to initialize the chase speed and set state to IDLE
EnemyController::EnemyController(float chaseSpeed)
    : m_chaseSpeed(chaseSpeed), m_state(EnemyState::IDLE)
{
}

void EnemyController::Init()
{
    // Grab references to the necessary components
    auto* pGameObject = GetGameObject();
    if (pGameObject)
    {
        m_pTransform = pGameObject->GetComponent<wolf::Transform2D>();
        m_pVelocity = pGameObject->GetComponent<VelocityComponent>();
        m_pHealth = pGameObject->GetComponent<HealthComponent>();
        m_pAnim = pGameObject->GetComponent<AnimatedSprite2D>();

        // Search for the player object in the scene and set it as the target
        for (auto&& [entity, playerController] : pGameObject->GetScene().Each<PlayerController>())
        {
            m_pTarget = playerController.GetGameObject();
            break; // Assume there's only one player in the scene
        }
    }
}

void EnemyController::Update(float delta)
{
    // Ensure components and target are initialized
    if (!m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget) return;

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
    if (IsPlayerInRange())
    {
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
    if (!m_pTarget || m_state == EnemyState::DEATH) return;

    MoveTowardsTarget(delta);

    // Transition back to idle if player is out of range
    if (!IsPlayerInRange())
    {
        m_state = EnemyState::IDLE;
        return;
    }

    // Update animation
    if (m_pAnim)
    {
        m_pAnim->SetAnimation("Walk");
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

    glm::vec2 direction = glm::normalize(targetPosition - currentPosition);

    // Move towards the target
    m_pVelocity->SetVelocity(direction * m_chaseSpeed);
}

bool EnemyController::IsPlayerInRange() const
{
    if (!m_pTransform || !m_pTarget) return false;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Calculate distance to the player
    float distance = glm::length(targetPosition - currentPosition);

    // Define detection range (can be a configurable variable)
    const float detectionRange = 300.0f; // Example value

    return distance <= detectionRange;
}
