#include "MinitaurController.h"
#include "PlayerController.h"
#include <cassert>

MinitaurController::MinitaurController() : EnemyController() {}

MinitaurController::~MinitaurController()
{
    m_pAnimComponent = nullptr;
    m_pTarget = nullptr;
    m_pVelocity = nullptr;
    m_pHealth = nullptr;
    m_pTransform = nullptr;
}

void MinitaurController::Init(const EnemyData& data)
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject)
    {
        wolf::Error("LateInitialize failed: MinitaurController not attached to GameObject!");
        return;
    }

    EnemyController::Init();  // Call the base enemy initialization
    m_meleeRange = data.meleeRange;
    m_attackCooldown = data.attackCooldown;
    m_detectionRange = data.detectionRange;
    m_baseDamage = data.baseDamage;
    m_chaseSpeed = data.chaseSpeed;

    // Get required components
    m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();

    // Set up Minitaur-specific animations
    SetUpAnimations(data.animationInitFile);

    // Find and set the player as the target
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        break;  // Assume there's only one player
    }
}


void MinitaurController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;


    // Update based on the current state
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
        // case EnemyState::DEATH:
        //     HandleDeathState();  // Temporarily disable death handling
        //     break;
    }

    // Update animations based on direction after handling movement
    UpdateAnimationBasedOnDirection();
}

void MinitaurController::SetUpAnimations(const std::string& animationInitPath)
{
    auto* pGameObject = GetGameObject();
    
    // Check if the AnimatedSprite2D component exists
    if (pGameObject->HasAll<AnimatedSprite2D>())
    {
        pGameObject->DeleteComponent<AnimatedSprite2D>();  // Use DeleteComponent to remove the existing component
        wolf::Warning("Removed existing anim component from minitaur...");
    }

    // Initialize the AnimatedSprite2D component
    m_pAnimComponent = &GetGameObject()->AddComponent<AnimatedSprite2D>(animationInitPath);
}

void MinitaurController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform) return;

    // Calculate the direction towards the player and move the Minitaur
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();

    // Calculate direction vector
    glm::vec2 direction = targetPosition - currentPosition;
    if (glm::length(direction) > 0.01f) {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
    } else {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

void MinitaurController::HandleIdleState()
{
    if (glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition()) < m_detectionRange)
    {
        ChangeState(EnemyState::CHASING);  // This should transition the enemy to the CHASING state
    }
}

void MinitaurController::HandleChasingState(float delta)
{
    MoveTowardsTarget(delta);

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    if (!m_transitionTimer.IsRunning())
    {
        m_transitionTimer.Start();
    }



    if (distanceToPlayer <= m_meleeRange)
    {
        if (m_transitionTimer.Elapsed() >= m_transitionDelay)
        {
            ChangeState(EnemyState::ATTACKING);
            m_transitionTimer.Reset();
        }
    }
    else
    {
        // Ensure the timer is reset if the player is not in range
        m_transitionTimer.Reset();
    }
}




void MinitaurController::HandleAttackingState(float delta)
{
    if (!m_pTarget) return;

    // Stop Minitaur's movement during attack
    m_pVelocity->SetVelocity(glm::vec2(0.0f));

    // Check distance to player
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    // Apply damage if player is within melee range and attack cooldown is over
    if (distanceToPlayer <= m_meleeRange && m_attackTimer <= 0.0f)
    {
        
        // Simulate applying damage to the player
        auto* playerHealth = m_pTarget->GetComponent<HealthComponent>();
        if (playerHealth)
        {
            playerHealth->Damage(m_baseDamage);  // Apply damage to the player
            std::cout << "Player Health: " << playerHealth->GetHealth() << "\n";
            wolf::Audio::Play("data/sounds/hurt.wav");

            // Reset attack cooldown timer
            m_attackTimer = m_attackCooldown;
        }
    }

    // Cooldown timer for next attack
    m_attackTimer -= delta;

    // Return to chasing if player moves out of range
    if (distanceToPlayer > m_meleeRange)
    {
        ChangeState(EnemyState::CHASING);
    }
}


void MinitaurController::UpdateAnimationBasedOnDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName;

    // Get the current velocity to determine direction
    glm::vec2 velocity = m_pVelocity->GetVelocity();

    // Only update animation if the Minitaur is moving
    if (glm::length(velocity) > 0.01f)  // Ensure the velocity is not zero
    {
        // Check if the movement is more along the X or Y axis
        if (fabs(velocity.x) > fabs(velocity.y))
        {
            // Moving left or right
            animationName = (velocity.x > 0.0f) ? "StandEast" : "StandWest";
        }
        else
        {
            // Moving up or down
            animationName = (velocity.y > 0.0f) ? "StandNorth" : "StandSouth";
        }
    }
    else
    {
        // If not moving, default to idle state based on the last direction
        animationName = "StandSouth";  // Modify as needed
    }

    // Check if the animation needs to be changed
    if (m_pAnimComponent->GetCurrentAnimation()->m_strName != animationName)
    {
        m_pAnimComponent->SetAnimation(animationName);
        m_pAnimComponent->SetOriginToCenterOfFrame();
    }
}

// void MinitaurController::HandleDeathState()
// {
//     // Destroy the GameObject when the Minitaur dies
//     std::cout << "Minitaur is being destroyed.\n";
//     GetGameObject()->Delete();
// }
