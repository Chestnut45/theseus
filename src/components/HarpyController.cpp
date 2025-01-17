#include "HarpyController.h"
#include "PlayerController.h"
#include "AttackDamageComponent.h"
#include "HomingComponent.h"
#include "TimedDestroyerComponent.h"

// !-- Aurora added this --!
#include "inventory/ItemDropCreator.h"

#include <math.h>
#include <cassert>

HarpyController::HarpyController()
{
}  

HarpyController::~HarpyController()
{
}

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
    m_rangedRange = data.rangedRange;
    m_rangedCooldown = data.rangedCooldown;
    m_detectionRange = data.detectionRange >= 0.0f ? data.detectionRange : std::numeric_limits<float>::infinity();
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

        // Init emotes object
    m_pEmoteObj = &pGameObject->GetScene().CreateObject2D();
    pGameObject->AddChild(*m_pEmoteObj);
    wolf::Transform2D* transform = m_pEmoteObj->GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(-8.0f, 8.0f));

    // Add emotes spritesheet
    AnimatedSprite2D* emotesSpritesheet = &m_pEmoteObj->AddComponent<AnimatedSprite2D>("data/emotes_anim_init.yaml");
    emotesSpritesheet->SetAnimPaused(true);
    emotesSpritesheet->SetOriginToCenterOfFrame();

    // Initialise emotes-related variables
    m_fEmoteTimer = EMOTE_TIME;
}


void HarpyController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;
    // Check if minitaur is petrified
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::DEATH);
    }

    // Check if health is below or equal to 0 and transition to the DEATH state
    if (m_pHealth->GetHealth() <= 0)
    {
        // Switch to the DEATH state if the health is depleted
        ChangeState(EnemyState::DEATH);
    }

    if(m_rangedTimer > 0.0f)
    {
        // Cooldown timer for next attack
        m_rangedTimer -= delta;
    }

    // Update based on the current state
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
        case EnemyState::PETRIFIED:
            HandlePetrifiedState(delta);
            break;
        case EnemyState::STUNNED:
            HandleStunnedState(delta);
            break;
        case EnemyState::DEATH:
            HandleDeathState(delta);
            return;  // After calling HandleDeathState(), return immediately since the object is now deleted
    }

    // Update animations based on direction after handling movement
    UpdateAnimationBasedOnDirection();

    // Emoting
    if(m_fEmoteTimer > 0.0f)
    {
        m_fEmoteTimer -= delta;
    }
    else
    {
        if(m_emote != EnemyEmote::NONE)
        {
            // Clear previous emote
            SetEmote(EnemyEmote::NONE);
        }
    }
}

void HarpyController::ChangeState(EnemyState newState)
{
     // Exit old state
    switch (m_state)
    {
        case EnemyState::ATTACKING:
        {
            ExitAttackState();
            break;
        }
        case EnemyState::CHASING:
        {
            ExitChasingState();
            break;
        }
        case EnemyState::IDLE:
        {
            ExitIdleState();
            break;
        }
        case EnemyState::PETRIFIED:
        {
            ExitPetrifiedState();
            break;
        }
        case EnemyState::STUNNED:
        {
            ExitStunnedState();
            break;
        }
        default:
        {
            break;
        }
    }
    // Enter new state
        switch (newState)
    {
        case EnemyState::CHASING:
        {
            EnterChasingState();
            break;
        }
        case EnemyState::IDLE:
        {
            EnterIdleState();
            break;
        }
        case EnemyState::PETRIFIED:
        {
            EnterPetrifiedState();
            break;
        }
        case EnemyState::STUNNED:
        {
            EnterStunnedState();
            break;
        }
        default:
        {         
            break;
        }
    }

    m_state = newState;
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

void HarpyController::HandleIdleState(float delta)
{
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);
    
    // If target detected, emote & chase
    if (distanceToPlayer <= m_detectionRange)
    {
        SetEmote(EnemyEmote::EXCLAMATION);
        ChangeState(EnemyState::CHASING);
    }
}

void HarpyController::HandleChasingState(float delta)
{
    MoveTowardsTarget(delta);

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    if(distanceToPlayer <= m_rangedRange)
    {
        if (m_transitionTimer.Elapsed() >= m_transitionDelay && m_rangedTimer <= 0.0f)
        {
            ChangeState(EnemyState::ATTACKING);            
        }
    }
}

void HarpyController::HandleAttackingState(float delta)
{
    if (!m_pTarget) return;
    
    // Retrieving & calculating data
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    glm::vec2 projectileDimensions = glm::vec2(10.0f, 10.0f);
    glm::vec2 hurtboxOffset = glm::vec2(-5.0f, 5.0f);
    glm::vec2 harpyDirection = targetPosition - currentPosition == glm::vec2(0.0f, 0.0f) ? glm::vec2(0.0f, 0.0f) : glm::normalize(targetPosition - currentPosition);
    glm::vec2 perpendicularVector = harpyDirection == glm::vec2(0.0f, 0.0f) ? glm::vec2(0.0f, 0.0f) : glm::normalize(glm::vec2(harpyDirection.y, -harpyDirection.x));
    glm::vec2 projectileDefaultVelocity = harpyDirection * 168.0f;

    auto& scene = this->GetGameObject()->GetScene();

    // Spawn 3 projectiles
    for(int i = -1; i <= 1; i += 1)
    {
        auto& projectile = scene.CreateObject2D();

        std::vector<std::pair<StatusComponent::StatusEffectType, float>> statusEffects = 
        {
            std::pair<StatusComponent::StatusEffectType, float>(StatusComponent::StatusEffectType::BURNING, 5.0f)
        };

        auto& attackDamageComponent = projectile.AddComponent<AttackDamageComponent>(m_baseDamage, m_pColliderManager, 0.0f, statusEffects);

        auto& projectileSprite = projectile.AddComponent<wolf::Sprite2D>("data/textures/Fireball.png");
        projectileSprite.SetOriginToCenterOfTexture();
        
        auto& projectileCollider = projectile.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
        projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
        projectileCollider.SetIgnoreTag(this->GetGameObject()->GetID());

        auto& projectileHoming = projectile.AddComponent<HomingComponent>(m_pTarget, 6.0f, 0.1f);
        auto& projectileTimedDestroyer = projectile.AddComponent<TimedDestroyerComponent>(10);

        auto& projectileVelocityComponent = projectile.AddComponent<VelocityComponent>();
        projectileVelocityComponent.SetVelocity(projectileDefaultVelocity);

        glm::vec2 offset = perpendicularVector * (30.0f * i);
        projectile.GetComponent<wolf::Transform2D>()->SetPosition(m_pTransform->GetGlobalPosition() + offset);
        projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));
    }
    ChangeState(EnemyState::CHASING);
}

void HarpyController::HandlePetrifiedState(float delta)
{
    ChangeState(EnemyState::DEATH);
}

void HarpyController::HandleStunnedState(float delta)
{
    AnimatedSprite2D* sprite = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
    if(m_stunnedTimer >= m_stunnedTime)
    {
        if(sprite != nullptr)
        {
            sprite->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
        }
        m_stunnedTimer = 0.0f;
        ChangeState(EnemyState::CHASING);
    }
    
    if(sprite != nullptr)
    {
        sprite->SetTint(glm::vec3(1.0f, 0.0f, 0.0f));
    }
    m_stunnedTimer += delta;
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
                collider->SetIgnoreTag(m_uiPlayerGOId);
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
            // Spawn some loot
            std::vector<wolf::GameObject*> pItemDrops = ItemDropCreator::Instance()->CreateItemDropFromLootTable("data/minitaur_loot.yaml", m_pTransform->GetGlobalPosition(), -1.0f);
            
            // Harpies can be inside of the walls so we need to push the loot out. To do that,
            // we get the loot item's velocity component
            for (auto& pItem : pItemDrops) {
                VelocityComponent* pItemVel = pItem->GetComponent<VelocityComponent>();
                if (pItemVel) {
                    // And gently push it in a random direction, which signals a collision in the ColliderManager
                    // that caluclates which direction the item should ACTUALLY be pushed in to get it out of the
                    // wall
                    pItemVel->ApplyKnockback(glm::vec2(1.0f, 0.0f), 10.0f);
                }
            }

            // Then we delete the harpy
            GetGameObject()->Delete();
        }
        m_lieDeadTimer += delta;
    }  
}

void HarpyController::EnterChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Start();
}
void HarpyController::EnterPetrifiedState()
{

}

void HarpyController::EnterIdleState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void HarpyController::EnterStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
}

void HarpyController::ExitAttackState()
{
    m_rangedTimer = m_rangedCooldown;
}

void HarpyController::ExitChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Stop();
    m_transitionDelay = m_RNG.NextFloat(0.8f, 1.6f);
}

void HarpyController::ExitIdleState()
{
}

void HarpyController::ExitPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_pAnimComponent->SetAnimPaused(false);
}

void HarpyController::ExitStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_stunnedTimer = 0.0f;
}

void HarpyController::SetEmote(EnemyEmote p_emote)
{
    m_fEmoteTimer = EMOTE_TIME;
    switch(p_emote)
    {
        case EnemyEmote::EXCLAMATION:
        {
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("Exclamation");
            break;
        }

        case EnemyEmote::QUESTION:
        {
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("Question");
            break;
        }
        case EnemyEmote::NONE:
        {
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("None");
            break;
        }
    }

    m_emote = p_emote;
}