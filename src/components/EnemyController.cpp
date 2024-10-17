#include "EnemyController.h"
#include <cassert>

EnemyController::EnemyController(float chaseSpeed)
    : m_chaseSpeed(chaseSpeed), m_state(EnemyState::IDLE)
{
}

void EnemyController::SetColliderManager(ColliderManager* pColliderManager)
{
    m_pColliderManager = pColliderManager;
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
        m_pCollider = pGameObject->GetComponent<ColliderComponent>();  // Get the ColliderComponent

        // Initialize the AnimatedSprite2D component
        if (!m_pAnimComponent)
        {
            m_pAnimComponent = &pGameObject->AddComponent<AnimatedSprite2D>("data/textures/Minitaur-Sheet.png", glm::vec2(32.0f, 32.0f), 4.0f);
        }
        SetUpAnimations();

        // Check if the essential components are initialized properly
        assert(m_pTransform != nullptr && m_pVelocity != nullptr && m_pHealth != nullptr && m_pCollider != nullptr && "Components not properly initialized in EnemyController!");

        // Search for the player object in the scene and set it as the target
        for (auto&& [entity, playerController] : pGameObject->GetScene().Each<PlayerController>())
        {
            m_pTarget = playerController.GetGameObject();
            break; // Assume there's only one player in the scene
        }

        assert(m_pTarget != nullptr && "No player found in the scene!");
    }
}

void EnemyController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    // Update the animation based on the movement direction and state
    UpdateAnimationBasedOnStateAndDirection();
    auto* enemyCollider = GetGameObject()->GetComponent<ColliderComponent>();

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
    if (!m_pTarget) return;

    // Calculate distance to player
    float distance = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());
    
    // Check if the player is within melee range to start attacking
    if (distance <= m_meleeRange) 
    {
        // Wait a short moment before attacking to make the transition smoother
        if (m_transitionTimer.Elapsed() >= m_transitionDelay)
        {
            m_state = EnemyState::ATTACKING;
            m_transitionTimer.Reset();
        }
        return;
    }

    // Smooth movement towards the player
    MoveTowardsTarget(delta);

    // Transition back to idle if player is out of detection range
    if (!IsPlayerInRange()) 
    {
        m_state = EnemyState::IDLE;
    }
}
void EnemyController::HandleAttackingState(float delta)
{
    if (!m_pTarget) return;

    // Calculate distance to the player
    float distance = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    // If the player moves out of melee range by a small tolerance, go back to chasing
    if (distance > m_meleeRange + 20.0f)  // Add a small tolerance to avoid jittering
    {
        std::cout << "Player moved out of range, switching to CHASING state!" << std::endl;
        m_state = EnemyState::CHASING;
        return;
    }

    // Stop or slow down the enemy's movement when attacking
    m_pVelocity->SetVelocity(glm::vec2(0.0f)); // Keep the enemy still during attack

    // Optional: Apply a slight "lunge" toward the player if out of direct melee range
    if (distance > m_meleeRange)
    {
        glm::vec2 lungeDirection = glm::normalize(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());
        m_pTransform->Translate(lungeDirection * m_chaseSpeed * 0.5f * delta); // Lunge towards the player
    }

    // Attack cooldown timer to control attack frequency
    m_attackTimer -= delta;

    // Ensure continuous collision check and apply damage if the enemy and player are colliding
    if (m_pColliderManager && m_pCollider && m_pTarget)
    {
        auto* playerCollider = m_pTarget->GetComponent<ColliderComponent>();
        if (playerCollider && m_pCollider->IsHitbox() && playerCollider->IsHurtbox())
        {
            if (m_pColliderManager->IsColliding(m_pCollider, playerCollider, 0.0f))
            {
                // Apply damage if cooldown has elapsed
                if (m_attackTimer <= 0.0f)
                {
                    ApplyDamageToPlayer();
                    m_attackTimer = m_attackCooldown;  // Reset the timer
                }
            }
        }
    }
}

EnemyController::~EnemyController()
{
    // Just reset the pointers, don't call any deletion methods.
    m_pAnimComponent = nullptr;
    m_pHealth = nullptr;
    m_pVelocity = nullptr;
    m_pTransform = nullptr;
    m_pTarget = nullptr;

    std::cout << "EnemyController destructor called." << std::endl;
}

void EnemyController::MoveTowardsTarget(float delta)
{
    if (!m_pTransform || !m_pTarget || !m_pVelocity) return;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    if (std::isnan(currentPosition.x) || std::isnan(currentPosition.y) || 
        std::isnan(targetPosition.x) || std::isnan(targetPosition.y))
    {
        return;
    }

    // Calculate the desired direction to the target
    glm::vec2 desiredDirection = glm::normalize(targetPosition - currentPosition);

    // Perform linear interpolation manually
    glm::vec2 newDirection = m_currentDirection + 0.1f * (desiredDirection - m_currentDirection);

    // Store the current direction for future reference
    m_currentDirection = newDirection;

    // Set the velocity based on the interpolated direction (do not multiply by delta)
    glm::vec2 velocity = m_currentDirection * m_chaseSpeed;

    // Set the velocity to move the enemy
    m_pVelocity->SetVelocity(velocity);
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
    if (!m_pColliderManager || !m_pCollider || !m_pTarget) return;

    // Get the player's ColliderComponent
    auto* playerCollider = m_pTarget->GetComponent<ColliderComponent>();
    if (!playerCollider) return;

    // Check for collision between the enemy's hitbox and the player's hurtbox
    if (m_pCollider->IsHitbox() && playerCollider->IsHurtbox())
    {
        // Use m_pColliderManager to check if the colliders are colliding
        if (m_pColliderManager->IsColliding(m_pCollider, playerCollider, 0.0f))
        {
            auto* playerHealth = m_pTarget->GetComponent<HealthComponent>();
            if (playerHealth)
            {
                playerHealth->Damage(m_baseDamage);
                std::cout << "Enemy attacked player!" << std::endl;
                std::cout << "Player HealthComponent - Damage: " << m_baseDamage << std::endl;
                std::cout << "Player HealthComponent - Health: " << playerHealth->GetHealth() << std::endl;
            }
        }
    }
}

void EnemyController::SetUpAnimations()
{
    if (!m_pAnimComponent) return;

    m_pAnimComponent->AddAnimation("StandWest", "data/textures/Minitaur-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 1, false);
    m_pAnimComponent->AddAnimation("StandSouth", "data/textures/Minitaur-Sheet.png", glm::vec2(32.0f, 32.0f), 2, 2, false);
    m_pAnimComponent->AddAnimation("StandEast", "data/textures/Minitaur-Sheet.png", glm::vec2(32.0f, 32.0f), 3, 3, false);
    m_pAnimComponent->AddAnimation("StandNorth", "data/textures/Minitaur-Sheet.png", glm::vec2(32.0f, 32.0f), 4, 4, false);

    // Set the default animation to face South
    m_pAnimComponent->SetAnimation("StandSouth");
    m_pAnimComponent->SetOriginToCenterOfFrame();
}

void EnemyController::UpdateAnimationBasedOnStateAndDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName;

    // Determine the animation based on movement and state
    switch (m_state)
    {
        case EnemyState::IDLE:
            animationName = "WalkSouth"; // Default idle animation
            break;

        case EnemyState::CHASING:
        {
            // Determine direction based on velocity
            glm::vec2 velocity = m_pVelocity->GetVelocity();
            if (glm::length(velocity) > 0.01f)  // Check if the enemy is moving
            {
                if (fabs(velocity.x) > fabs(velocity.y))
                {
                    animationName = (velocity.x > 0.0f) ? "StandEast" : "StandWest";
                }
                else
                {
                    animationName = (velocity.y > 0.0f) ? "StandNorth" : "StandSouth";
                }
            }
            break;
        }

        case EnemyState::ATTACKING:
            animationName = "WalkSouth";  // Modify as needed if attack animations are added later
            break;

    }

    // Check if the animation needs to be changed
    SpriteAnimation2D* currentAnim = m_pAnimComponent->GetCurrentAnimation();
    if (!currentAnim || currentAnim->m_strName != animationName)
    {
        m_pAnimComponent->SetAnimation(animationName);
    }
}
