#include "HarpyController.h"
#include "PlayerController.h"
#include "AttackDamageComponent.h"
#include "HomingComponent.h"
#include "TimedDestroyerComponent.h"

// !-- Aurora added this --!
#include "inventory/ItemDropCreator.h"

#include <math.h>
#include <cassert>



void HarpyController::Init(const EnemyData& data)
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject)
    {
        wolf::Error("LateInitialize failed: HarpyController not attached to GameObject!");
        return;
    }

    // Call base initialization
    EnemyController::Init();

    // Assign enemy data
    m_meleeRange = data.meleeRange;
    m_attackCooldown = data.attackCooldown;
    m_detectionRange = data.detectionRange;
    m_baseDamage = data.baseDamage;
    m_chaseSpeed = data.chaseSpeed;

    // Get required components and log their initialization
    m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (!m_pVelocity)
    {
        wolf::Warning("Harpy " + std::to_string(pGameObject->GetID()) + " could not find VelocityComponent!");
    }

    // Set up Harpy-specific animations
    SetUpAnimations(data.animationInitFile);

    // Find and set the player as the target
    bool targetFound = false;
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        targetFound = true;
        break;  // Assume there's only one player
    }

    if (!targetFound)
    {
        wolf::Warning("Harpy " + std::to_string(pGameObject->GetID()) + " did not find any player target!");
    }
}


void HarpyController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    // Check if health is below or equal to 0 and transition to the DEATH state
    if (m_pHealth->GetHealth() <= 0)
    {
        // Switch to the DEATH state if the health is depleted
        ChangeState(EnemyState::DEATH);
    }

    if(m_attackTimer > 0.0f)
    {
        // Cooldown timer for next attack
        m_attackTimer -= delta;
    }

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
        case EnemyState::DEATH:
            HandleDeathState(delta);
            return;  // After calling HandleDeathState(), return immediately since the object is now deleted
    }

    // Update animations based on direction after handling movement
    UpdateAnimationBasedOnDirection();
}


void HarpyController::SetUpAnimations(const std::string& animationInitPath)
{
    auto* pGameObject = GetGameObject();
    
    // Check if the AnimatedSprite2D component exists
    if (pGameObject->HasAll<AnimatedSprite2D>())
    {
        pGameObject->DeleteComponent<AnimatedSprite2D>();  // Use DeleteComponent to remove the existing component
        wolf::Warning("Removed existing anim component from Harpy...");
    }

    // Initialize the AnimatedSprite2D component
    m_pAnimComponent = &GetGameObject()->AddComponent<AnimatedSprite2D>(animationInitPath);
    m_pAnimComponent->SetTint(glm::vec3(0,1,0));
}

void HarpyController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform) return;

    // Calculate the direction towards the player and move the Harpy
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();

    // Calculate direction vector
    glm::vec2 direction = targetPosition - currentPosition;

    // Log for debugging current position, target position, and distance
    // printf("Harpy MoveTowardsTarget: Current Pos: (%f, %f), Target Pos: (%f, %f)\n", 
    //        currentPosition.x, currentPosition.y, targetPosition.x, targetPosition.y);

    if (glm::length(direction) > 0.01f) {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);

        // Log the velocity being set
        // printf("Velocity Set: (%f, %f)\n", direction.x * m_chaseSpeed, direction.y * m_chaseSpeed);
    } else {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
        // printf("Velocity Stopped\n");
    }
}

void HarpyController::HandleIdleState()
{
    // Check if the player is within detection range
    float distanceToPlayer = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    // If the player comes into detection range, start chasing
    if (distanceToPlayer <= m_detectionRange)
    {
        ChangeState(EnemyState::CHASING);  // Transition to CHASING when the player is in range
    }
}

void HarpyController::HandleChasingState(float delta)
{
    MoveTowardsTarget(delta);
    // glm::vec2 currentVelocity = m_pVelocity->GetVelocity();
    // printf("After MoveTowardsTarget - Velocity: (%f, %f)\n", currentVelocity.x, currentVelocity.y);

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    // Check if the player has moved out of the detection range and transition to IDLE
    if (distanceToPlayer > m_detectionRange)
    {
        ChangeState(EnemyState::IDLE);
        m_pVelocity->SetVelocity(glm::vec2(0.0f));  // Reset velocity when returning to idle
        return;
    }

    if (!m_transitionTimer.IsRunning())
    {
        m_transitionTimer.Start();
    }

    // if (distanceToPlayer <= m_meleeRange)
    // {
    //     if (m_transitionTimer.Elapsed() >= m_transitionDelay)
    //     {
    //         ChangeState(EnemyState::ATTACKING);
    //         m_transitionTimer.Reset();
    //     }
    // }
    if(distanceToPlayer <= m_rangedRange)
    {
        if (m_transitionTimer.Elapsed() >= m_transitionDelay && m_attackTimer <= 0.0f)
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

void HarpyController::HandleAttackingState(float delta)
{
    if (!m_pTarget) return;

    // Check distance to player
    
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    glm::vec2 projectileDimensions = glm::vec2(10.0f, 10.0f);
    glm::vec2 hurtboxOffset = glm::vec2(-5.0f, 5.0f);
    glm::vec2 harpyDirection = targetPosition - currentPosition == glm::vec2(0.0f, 0.0f) ? glm::vec2(0.0f, 0.0f) : glm::normalize(targetPosition - currentPosition);
    glm::vec2 perpendicularVector = harpyDirection == glm::vec2(0.0f, 0.0f) ? glm::vec2(0.0f, 0.0f) : glm::normalize(glm::vec2(harpyDirection.y, -harpyDirection.x));
    glm::vec2 projectileDefaultVelocity = harpyDirection * 168.0f;

    auto& scene = this->GetGameObject()->GetScene();
    
    for(int i = -1; i <= 1; i += 1)
    {
        auto& projectile = scene.CreateObject2D();

        // auto& attackDamageComponent = projectile.AddComponent<AttackDamageComponent>(100.0f, m_pColliderManager);
        // Add AttackDamageComponent with knockback
        auto& attackDamageComponent = projectile.AddComponent<AttackDamageComponent>(100.0f, m_pColliderManager, 200.0f); // 200.0f is knockback magnitude

        auto& projectileSprite = projectile.AddComponent<wolf::Sprite2D>("data/textures/Fireball.png");
        projectileSprite.SetOriginToCenterOfTexture();
        
        auto& projectileCollider = projectile.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
        projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
        projectileCollider.SetIgnoreTag(this->GetGameObject()->GetID());

        auto& projectileHoming = projectile.AddComponent<HomingComponent>(m_pTarget, 12.0f, 16);
        auto& projectileTimedDestroyer = projectile.AddComponent<TimedDestroyerComponent>(10);

        auto& projectileVelocityComponent = projectile.AddComponent<VelocityComponent>();
        // float angle = (60 * -i) / (MATH_PI * 180.0f);
        // glm::vec2 projectileVelocity = glm::vec2(0.0f, 0.0f);
        // projectileVelocity.x = projectileDefaultVelocity.x * glm::cos(angle) - projectileDefaultVelocity.y * glm::sin(angle);
        // projectileVelocity.y = projectileDefaultVelocity.x * glm::sin(angle) + projectileDefaultVelocity.y * glm::cos(angle);
        projectileVelocityComponent.SetVelocity(projectileDefaultVelocity);

        glm::vec2 offset = perpendicularVector * (30.0f * i);
        projectile.GetComponent<wolf::Transform2D>()->SetPosition(m_pTransform->GetGlobalPosition() + offset);
        projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));
    }
    
    
    m_attackTimer = m_attackCooldown;
    

    ChangeState(EnemyState::CHASING);
}



void HarpyController::UpdateAnimationBasedOnDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName;

    // Get the current velocity to determine direction
    glm::vec2 velocity = m_pVelocity->GetVelocity();

    // Only update animation if the Harpy is moving
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

void HarpyController::HandleDeathState(float delta)
{
     // Fall over
    if(m_fallDeadTimer <= m_timeToFallDead)
    {
        if(m_fallDeadTimer == 0.0f)
        {
            if (m_pVelocity)
            {
                m_pVelocity->SetVelocity(glm::vec2(0.0f));
            }

            ColliderComponent* collider = this->GetGameObject()->GetComponent<ColliderComponent>();
            if(collider != nullptr)
            {
                collider->SetColliderType(ColliderComponent::ColliderType::NONE);
            }
            
            m_pAnimComponent->SetTint(glm::vec3(1,0,0));
        }

        float angle = (90.0f / m_timeToFallDead) * delta;
        m_pTransform->RotateDegrees(angle);
        
        m_fallDeadTimer += delta;
    }

    // Lie dead
    else
    {
        if(m_lieDeadTimer >= m_timeToLieDead)
        {
            // !-- Aurora added this --!
            ItemDropCreator::Instance()->CreateItemDropFromLootTable("data/minitaur_loot.yaml", m_pTransform->GetGlobalPosition(), 5.0f);
            GetGameObject()->Delete();
        }
        m_lieDeadTimer += delta;
    }  
}

void HarpyController::ChangeState(EnemyState newState)
{
    m_state = newState;
}