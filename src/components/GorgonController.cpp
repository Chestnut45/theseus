#include "GorgonController.h"
#include "PlayerController.h"
#include "LabyrinthManager.h"
#include "../GLShapesRenderer.h"
#include "../DDACalculator.h"

#include <cassert>

GorgonController::GorgonController()
{
}   

GorgonController::~GorgonController()
{
}

void GorgonController::Init(const EnemyData& data)
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject)
    {
        wolf::Error("LateInitialize failed: GorgonController not attached to GameObject!");
        return;
    }

    // Call base initialization
    EnemyController::Init();
    m_transitionDelay = m_RNG.NextFloat(0.8f, 1.6f);

    // Assign enemy data
    m_meleeRange = data.meleeRange;
    m_rangedRange = data.rangedRange;
    m_rangedCooldown = data.rangedCooldown;
    m_rangedWindupTime = data.rangedWindup;
    m_detectionRange = data.detectionRange;
    m_baseDamage = data.baseDamage;
    m_chaseSpeed = data.chaseSpeed;

    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);

    // Set attack timer
    m_rangedTimer = m_rangedCooldown;

    // Get required components and log their initialization
    m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (!m_pVelocity)
    {
        wolf::Warning("Gorgon " + std::to_string(pGameObject->GetID()) + " could not find VelocityComponent!");
    }

    // Set up Gorgon-specific animations
    SetUpAnimations(data.animationInitFile);

    // Find and set the player as the target
    bool targetFound = false;
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        m_pTargetStatusComponent = m_pTarget->GetComponent<StatusComponent>();
        targetFound = true;
        break;  // Assume there's only one player
    }

    if (!targetFound)
    {
        wolf::Warning("Gorgon " + std::to_string(pGameObject->GetID()) + " did not find any player target!");
    }

    // Init emotes object
    m_pEmoteObj = &pGameObject->GetScene().CreateObject2D();
    pGameObject->AddChild(*m_pEmoteObj);
    wolf::Transform2D* transform = m_pEmoteObj->GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(-8.0f, 8.0f));

    // Init attack state members
    m_rangedWindupTimer = m_rangedWindupTime;

    // Add emotes spritesheet
    AnimatedSprite2D* emotesSpritesheet = &m_pEmoteObj->AddComponent<AnimatedSprite2D>("data/emotes_anim_init.yaml");
    emotesSpritesheet->SetAnimPaused(true);
    emotesSpritesheet->SetOriginToCenterOfFrame();

    // Initialise emotes-related variables
    m_fEmoteTimer = EMOTE_TIME;
}


void GorgonController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_active || !m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;
    
    // Update the base class
    EnemyController::Update(delta);

    // Check if gorgon is petrified
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::PETRIFIED);
        return;
    }
    else 
    {
        // On exiting petrified state
        if(m_state == EnemyState::PETRIFIED)
        {
            ChangeState(EnemyState::CHASING);
            return;
        }         
    }

    // Check if health is below or equal to 0 and not already dying, transition to the DEATH state
    if (m_pHealth->GetHealth() <= 0 && m_state != EnemyState::DEATH)
    {
        // Switch to the DEATH state if the health is depleted
        ChangeState(EnemyState::DEATH);
        return;
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
        case EnemyState::PROSPECT:
            HandleProspectState(delta);
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
        // Clear previous emote
        if(m_emote != EnemyEmote::NONE)
        {
            SetEmote(EnemyEmote::NONE);
        }
    }
}

void GorgonController::ChangeState(EnemyState newState)
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
        case EnemyState::PROSPECT:
        {
            ExitProspectState();
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
        case EnemyState::ATTACKING:
        {
            EnterAttackState();
            break;
        }
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
        case EnemyState::PROSPECT:
        {
            EnterProspectState();
            break;
        }
        case EnemyState::STUNNED:
        {
            EnterStunnedState();
            break;
        }
        case EnemyState::DEATH:
        {
            EnterDeathState();
            break;
        }
        default:
        {         
            break;
        }
    }
    m_last_state = m_state;
    m_state = newState;
}

void GorgonController::SetUpAnimations(const std::string& animationInitPath)
{
    auto* pGameObject = GetGameObject();
    
    // Check if the AnimatedSprite2D component exists
    if (pGameObject->HasAll<AnimatedSprite2D>())
    {
        pGameObject->DeleteComponent<AnimatedSprite2D>();  // Use DeleteComponent to remove the existing component
        wolf::Warning("Removed existing anim component from Gorgon...");
    }

    // Initialize the AnimatedSprite2D component
    m_pAnimComponent = &GetGameObject()->AddComponent<AnimatedSprite2D>(animationInitPath);
}

void GorgonController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform) return;

    // Calculate the direction towards the player and move the Gorgon
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();

    // Calculate direction vector
    glm::vec2 direction = targetPosition - currentPosition;

    // Log for debugging current position, target position, and distance
    // printf("Gorgon MoveTowardsTarget: Current Pos: (%f, %f), Target Pos: (%f, %f)\n", 
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

void GorgonController::HandleIdleState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {
        // If target detected, emote & chase
        if (IsTargetDetected())
        {
            SetEmote(EnemyEmote::EXCLAMATION);
            ChangeState(EnemyState::CHASING); 
            return;
        }

        // Else, reset detection timer
        else
        {
            m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
        }
    }
    else
    {
        m_targetDetectionTimer -= delta;
    }
}

void GorgonController::HandleProspectState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {    
        // If target detected, emote & chase
        if (IsTargetDetected())
        {
            SetEmote(EnemyEmote::EXCLAMATION);
            ChangeState(EnemyState::CHASING);
            return; 
        }
        // Else, reset detection timer
        else
        {
            m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
        }
    }
    // Else, do thing
    else
    {
        m_targetDetectionTimer -= delta;
        // If pondering done, move
        if (m_prospectStandingCounter <= 0.0f)
        {
            // If moving done, decide next action
            if(m_prospectCounter <= 0)
            {          
                    // Roll for action
                    float rng = m_RNG.NextInt(1, 100);
                    // If larger than 10, prospect
                    if(rng > 10)
                    {
                        
                        m_prospectCounter = m_RNG.NextInt(1, 3);
                        glm::vec2 direction = glm::normalize(glm::vec2(m_RNG.NextInt(-100, 100), m_RNG.NextInt(-100, 100)));
                        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                    }

                    // Else, idle
                    else
                    {
                        ChangeState(EnemyState::IDLE);
                        return;
                    }
                    m_prospectStandingCounter = m_RNG.NextFloat(0.5f, 2.0f);                  
            }
            else
            {
                m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));
                m_prospectCounter -= delta;
            }
        }
        else
        {
        m_prospectStandingCounter -= delta;
        }
    }
}

void GorgonController::HandleChasingState(float delta)
{
    MoveTowardsTarget(delta);
    // glm::vec2 currentVelocity = m_pVelocity->GetVelocity();
    // printf("After MoveTowardsTarget - Velocity: (%f, %f)\n", currentVelocity.x, currentVelocity.y);

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToTarget = glm::length(targetPosition - currentPosition);

    // If player is out of detection range
    if(distanceToTarget > m_detectionRange)
    {
        // If player is not petrified, emote
        if
        (
            m_pTargetStatusComponent != nullptr                                                             && 
            !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)
        )
        {
            SetEmote(EnemyEmote::QUESTION);
        }

        // Switch to prospect
        ChangeState(EnemyState::PROSPECT);
        return;
    }

    // Else if player is within detection range
    else
    {
        // If player is petrified, switch to prospect
        if
        ( 
            m_pTargetStatusComponent != nullptr                                                             && 
            m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)
        )
        {
            ChangeState(EnemyState::PROSPECT);
            return;
        }
    }

    // If target is within ranged range
    if (distanceToTarget <= m_rangedRange)
    {
        if 
        (
            m_pTargetStatusComponent != nullptr                                                             &&  // If target status component not null
            m_transitionTimer.Elapsed() >= m_transitionDelay                                                &&  // If transition delay expired
            m_rangedTimer <= 0.0f                                                                           &&  // If delay between attacks expired
            !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)   &&  // If target not already petrified
            IsTargetInLOS()                                                                                     // If target in line of sight
        )
        {
            ChangeState(EnemyState::ATTACKING);
            return;
        }
    }
}

void GorgonController::HandleAttackingState(float delta)
{
    if (!m_pTarget)
    {
        ChangeState(EnemyState::IDLE);
        return;
    }

    // If winding up attack
    if(m_rangedWindupTimer > 0.0f)
    {
        m_rangedWindupTimer -= delta;
        
        // Brighten sprite to indicate attack
        if(m_pAnimComponent != nullptr)
        {
            glm::vec3 currentTint = m_pAnimComponent->GetTint();
            glm::vec3 nextTint = currentTint + glm::vec3(delta / (m_rangedWindupTime * 0.5f));
            m_pAnimComponent->SetTint(nextTint);
        }
    }
    
    // Else, strike
    else
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f)); // Reset windup tint

        // Determine if target's collider is active and a hurtbox
        auto* pCollider = m_pTarget->GetComponent<ColliderComponent>();
        const bool active = pCollider ? (pCollider->IsActive() && pCollider->IsHurtbox()) : false;

        // If target is in line of sight, petrify target and switch to prospect
        if(active && m_pTargetStatusComponent && IsTargetInLOS())
        {
            m_pTargetStatusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 5.0f);
        }
        
        ChangeState(EnemyState::CHASING);
        return;
    }

    m_curentCrosshairColour.g -= delta * (1.0f / m_rangedCooldown);
    RenderIndicator();
}

void GorgonController::HandlePetrifiedState(float delta)
{
    m_pAnimComponent->SetAnimPaused(true);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));    
}

void GorgonController::HandleStunnedState(float delta)
{
    if(m_stunnedTimer >= m_stunnedTime)
    {
        if
        (IsTargetDetected())
        {
            if
            (
            m_last_state != EnemyState::CHASING     &&
            m_last_state != EnemyState::ATTACKING
            )
            {
                SetEmote(EnemyEmote::EXCLAMATION);
            }
            ChangeState(EnemyState::CHASING);
            return;
        }
        
        SetEmote(EnemyEmote::QUESTION);
        ChangeState(EnemyState::PROSPECT);
        return;
    }
    else
    {
        //m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
        m_stunnedTimer += delta;
    }
}

void GorgonController::UpdateAnimationBasedOnDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName = "";

    // Get the current velocity to determine direction
    glm::vec2 velocity = m_pVelocity->GetVelocity();

    // Only update animation if the Gorgon is moving
    switch (m_state)
    {
        case EnemyState::ATTACKING:
        {
            if(m_state == EnemyState::ATTACKING)
            {
                const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
                const glm::vec2 vectorToTarget = targetPosition - currentPosition;
                const float distanceToTarget = glm::length(vectorToTarget);

                if (fabs(vectorToTarget.x) > fabs(vectorToTarget.y))
                {
                    // Moving left or right
                    animationName = (vectorToTarget.x > 0.0f) ? "StandEast" : "StandWest";
                }
                else
                {
                    // Moving up or down
                    animationName = (vectorToTarget.y > 0.0f) ? "StandNorth" : "StandSouth";
                }
            }
        }

        case EnemyState::CHASING:
        {
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
            break;
        }

        case EnemyState::PROSPECT:
        {
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
            break;
        }

        default:
        {
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
            break;
        }
    }

    // Check if the animation needs to be changed
    if (m_pAnimComponent->GetCurrentAnimation()->m_strName != animationName)
    {
        m_pAnimComponent->SetAnimation(animationName);
        m_pAnimComponent->SetOriginToCenterOfFrame();
    }
}

void GorgonController::HandleDeathState(float delta)
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
            // Spawn some loot
            std::vector<wolf::GameObject*> pItemDrops = ItemDropCreator::Instance().CreateItemDropFromLootTable("data/minitaur_loot.yaml", m_pTransform->GetGlobalPosition(), -1.0f);
            
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
                ColliderComponent* pItemCollider = pItem->GetComponent<ColliderComponent>();
                if (pItemCollider)
                {
                    // Disable the collider after knockback
                    pItemCollider->SetActive(false);
                }
            }
            GetGameObject()->Delete();
        }
        m_lieDeadTimer += delta;
    }
}

void GorgonController::EnterAttackState()
{
    m_IsRenderingAttackIndicator = true;
    m_crosshairOffset = glm::vec2(
        m_RNG.NextFloat(-4.0f, 4.0f),
        m_RNG.NextFloat(-4.0f, 4.0f)
        );
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void GorgonController::EnterChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Start();
}

void GorgonController::EnterIdleState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void GorgonController::EnterPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::MULTITEX_PETRIFIED);
}

void GorgonController::EnterProspectState()
{
}

void GorgonController::EnterStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
}

void GorgonController::EnterDeathState()
{
    m_IsRenderingAttackIndicator = false;
    SetEmote(EnemyEmote::NONE);
}

void GorgonController::ExitAttackState()
{   
    m_curentCrosshairColour = CROSSHAIR_COLOUR;

    if(m_pAnimComponent != nullptr)
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }

    m_rangedTimer = m_rangedCooldown;
    m_rangedWindupTimer = m_rangedWindupTime;
}

void GorgonController::ExitChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Stop();
    m_transitionDelay = m_RNG.NextFloat(0.8f, 1.6f);
}

void GorgonController::ExitIdleState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
}

void GorgonController::ExitPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_pAnimComponent->SetAnimPaused(false);
}

void GorgonController::ExitProspectState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
    m_prospectCounter = 0;
    m_prospectStandingCounter = m_RNG.NextFloat(0.5f, 2.0f);
}

void GorgonController::ExitStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_stunnedTimer = 0.0f;
}

void GorgonController::SetEmote(EnemyEmote p_emote)
{
    // std::cout << "GorgonController - p_emote: " << p_emote << std::endl;
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

bool GorgonController::IsTargetDetected()
{
    float distanceToTarget = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    if(
        m_pTargetStatusComponent != nullptr                                                             &&  // Target status component not null
        distanceToTarget <= m_detectionRange                                                            &&  // Target in detection range
        !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)   &&  // Target not already petrified
        IsTargetInLOS()                                                                                     // Target in line of sight
        )
    {
        return true;
    }
    return false;
}

bool GorgonController::IsTargetInLOS()
{
    glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = this->m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    if(DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos) == targetPos)
    {
        return true;
    }

    return false;
    
}

void GorgonController::RenderIndicator()
{
    glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = this->m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 endPos = DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos);
    glm::vec4 colour = this->m_curentCrosshairColour;
    
    DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos);
    GLShapesRenderer::GetInstance()->AddLine(
                                            {thisPos.x, thisPos.y, colour.r, colour.g, colour.b, colour.a},
                                            {endPos.x, endPos.y, colour.r, colour.g, colour.b, colour.a}
                                            );
}