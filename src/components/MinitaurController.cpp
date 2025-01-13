#include "MinitaurController.h"
#include "PlayerController.h"
#include "LabyrinthManager.h"

#include <cassert>

// !- Aurora added this --!
#include "inventory/ItemDropCreator.h"

MinitaurController::MinitaurController()
{
}

MinitaurController::~MinitaurController()
{
}

void MinitaurController::Init(const EnemyData& data)
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject)
    {
        wolf::Error("LateInitialize failed: MinitaurController not attached to GameObject!");
        return;
    }

    // Call base initialization
    EnemyController::Init();

    // Assign enemy data
    m_meleeRange = data.meleeRange;
    m_meleeCooldown = data.attackCooldown;
    m_detectionRange = data.detectionRange;
    m_baseDamage = data.baseDamage;
    m_chaseSpeed = data.chaseSpeed;

    // Set attack timer
    m_meleeTimer = m_meleeCooldown;

    // Get required components and log their initialization
    m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (!m_pVelocity)
    {
        wolf::Warning("Minitaur " + std::to_string(pGameObject->GetID()) + " could not find VelocityComponent!");
    }

    // Set up Minitaur-specific animations
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
        wolf::Warning("Minitaur " + std::to_string(pGameObject->GetID()) + " did not find any player target!");
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


void MinitaurController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    // Check if minitaur is petrified
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::PETRIFIED);
    }
   
    else 
    {
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
        // On exiting petrified state
        if(m_state == EnemyState::PETRIFIED)
        {
            m_state = EnemyState::IDLE;
            m_pAnimComponent->SetAnimPaused(false);
        }         
    }

    // Check if health is below or equal to 0 and transition to the DEATH state
    if (m_pHealth->GetHealth() <= 0)
    {
        // Switch to the DEATH state if the health is depleted
        ChangeState(EnemyState::DEATH);
    }

    // Update based on the current state
    switch (m_state)
    {
        case EnemyState::IDLE:
            HandleIdleState(delta);
            break;
        case EnemyState::PROSPECT:
            HandleProspectState(delta);
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

void MinitaurController::ChangeState(EnemyState newState)
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

void MinitaurController::HandleIdleState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {
        // If target detected, emote & chase
        if (IsTargetDetected())
        {
            SetEmote(EnemyEmote::EXCLAMATION);
            ChangeState(EnemyState::CHASING); 
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

void MinitaurController::HandleProspectState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {    
        // If target detected, emote & chase
        if (IsTargetDetected())
        {
            SetEmote(EnemyEmote::EXCLAMATION);
            ChangeState(EnemyState::CHASING); 
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
                    m_prospectCounter = m_RNG.NextInt(100, 200);
                    glm::vec2 direction = glm::normalize(glm::vec2(m_RNG.NextInt(-100, 100), m_RNG.NextInt(-100, 100)));
                    m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                }

                // Else, change to idle
                else
                {
                    ChangeState(EnemyState::IDLE);
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

void MinitaurController::HandleChasingState(float delta)
{
    MoveTowardsTarget(delta);

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    // If player is out of detection range, emote & switch to prospect
    if (distanceToPlayer > m_detectionRange)
    {
        SetEmote(EnemyEmote::QUESTION);
        ChangeState(EnemyState::PROSPECT);      
        return;
    }
    if (distanceToPlayer <= m_meleeRange)
    {
        if (m_transitionTimer.Elapsed() >= m_transitionDelay)
        {
            ChangeState(EnemyState::ATTACKING);
            return;
        }
    }
}

void MinitaurController::HandleAttackingState(float delta)
{
    if (!m_pTarget) return;

    // Stop Minitaur's movement during attack
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
    if(m_meleeTimer <= 0.0f)
    {
        // Check distance to player
        const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
        const float distanceToPlayer = glm::length(targetPosition - currentPosition);

        // Apply damage if player is within melee range and attack cooldown is over
        if (distanceToPlayer <= m_meleeRange)
        {
            // Apply damage to the player
            auto* playerHealth = m_pTarget->GetComponent<HealthComponent>();
            if (playerHealth)
            {
                playerHealth->Damage(m_baseDamage);
                wolf::Audio::Play("data/sounds/hurt.wav");

            }

            // Apply strong knockback to the player
            auto* playerVelocity = m_pTarget->GetComponent<VelocityComponent>();
            if (playerVelocity)
            {
                // Calculate knockback direction and amplify the push
                glm::vec2 knockbackDirection = glm::normalize(targetPosition - currentPosition);
                float knockbackStrength = 800.0f; // Amplified knockback strength
                playerVelocity->ApplyKnockback(knockbackDirection, knockbackStrength);
            }

        }
        else
        {
            // Return to chasing if player moves out of range
            ChangeState(EnemyState::CHASING);
        }

        // Reset attack cooldown timer
        m_meleeTimer = m_meleeCooldown;
    }
    else
    {
        m_meleeTimer -= delta;
    }
}

void MinitaurController::HandlePetrifiedState(float delta)
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::PETRIFIED);
    m_pAnimComponent->SetAnimPaused(true);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));  
}

void MinitaurController::HandleStunnedState(float delta)
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
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
        m_stunnedTimer += delta;
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

void MinitaurController::HandleDeathState(float delta)
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
            ItemDropCreator::Instance()->CreateItemDropFromLootTable("data/minitaur_loot.yaml", m_pTransform->GetGlobalPosition(), -1.0f);
            GetGameObject()->Delete();
        }
        m_lieDeadTimer += delta;

    }  
}

void MinitaurController::EnterChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Start();
}

void MinitaurController::EnterIdleState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void MinitaurController::EnterProspectState()
{
}

void MinitaurController::EnterStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
}
void MinitaurController::EnterDeathState()
{
    SetEmote(EnemyEmote::NONE);
}

void MinitaurController::ExitAttackState()
{
    m_meleeTimer = m_meleeCooldown;
    
    if(m_pAnimComponent != nullptr)
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }
}

void MinitaurController::ExitChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Stop();
    m_transitionDelay = m_RNG.NextFloat(0.8f, 1.6f);
}

void MinitaurController::ExitIdleState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
}

void MinitaurController::ExitPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_pAnimComponent->SetAnimPaused(false);
}

void MinitaurController::ExitProspectState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
    m_prospectCounter = 0;
    m_prospectStandingCounter = m_RNG.NextFloat(0.5f, 2.0f);
}

void MinitaurController::ExitStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_stunnedTimer = 0.0f;
}

void MinitaurController::SetEmote(EnemyEmote p_emote)
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

bool MinitaurController::IsTargetDetected()
{
    float distanceToTarget = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    if(
        distanceToTarget <= m_detectionRange    &&  // Target in detection range
        IsTargetInLOS()                             // Target in line of sight
        )
    {
        return true;
    }
    return false;
}

bool MinitaurController::IsTargetInLOS()
{

    // Get labyrinth manager
    LabyrinthManager* lbmg = nullptr;
    for (auto&& [_, labyrinthManager] : GetGameObject()->GetScene().Each<LabyrinthManager>())
    {
        lbmg = &labyrinthManager;
        break;
    }

    // Check
    if(lbmg != nullptr)
    {
        glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
        glm::ivec2 thisTilePos = lbmg->GetTilePosition(thisPos);
        int thisTileID = lbmg->GetTile(thisTilePos.x, thisTilePos.y);

        glm::vec2 targetPos = this->m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::ivec2 targetTilePos = lbmg->GetTilePosition(targetPos);
        int targetTileID = lbmg->GetTile(targetTilePos.x, targetTilePos.y);

        // If either this tile or target tile is invalid, return false
        if(thisTileID < 0 || targetTileID < 0)
        {
            // printf("Minitaur Controller - ERROR: INVALID TILE\n");
            return false;
        }

        // If either entity or target is inside wall (somehow), return false
        if(this->IsWallTile(thisTileID) || this->IsWallTile(targetTileID))
        {
            return false;
        }

        if(thisTilePos == targetTilePos && !this->IsWallTile(thisTileID))
        {
            // printf("Minitaur Controller - Target Detected\n");
            return true;
        }

        const int tileSize = (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);

        glm::vec2 line = targetPos - thisPos;
        glm::vec2 normalisedLine = glm::normalize(line);
        float distance = glm::length(line);

        glm::ivec2 currentTilePos = thisTilePos;
        int currentTileID = thisTileID;
        glm::vec2 rayLength = glm::vec2(0.0f, 0.0f);
        glm::ivec2 tileStep = glm::vec2(0, 0);
        glm::vec2 rayStep = glm::vec2(
            sqrt(1 + pow((normalisedLine.y / normalisedLine.x), 2)),
            sqrt(1 + pow((normalisedLine.x / normalisedLine.y), 2))
        );

        if(line.x > 0.0f)
        {
            tileStep.x = 1;
            rayLength.x = abs(this->GetTileWorldPos(glm::ivec2(thisTilePos.x, thisTilePos.y)).x - thisPos.x) * rayStep.x;
        }
        else
        {
            tileStep.x = -1;
            rayLength.x = abs(thisPos.x - this->GetTileWorldPos(glm::ivec2(thisTilePos.x, thisTilePos.y)).x) * rayStep.x;
        }

        if(line.y > 0.0f)
        {
            tileStep.y = 1;
            rayLength.y = abs(this->GetTileWorldPos(glm::ivec2(thisTilePos.x, thisTilePos.y)).y - thisPos.y) * rayStep.y;
        }
        else
        {
            tileStep.y = -1;
            rayLength.y = abs(thisPos.y - this->GetTileWorldPos(glm::ivec2(thisTilePos.x, thisTilePos.y)).y) * rayStep.y;
        }
        
        rayStep *= tileSize;
        
        // Iterate until target tile is reached
        bool isTargetSpotted = true;
        bool isIterating = true;
        float distanceCheck = 0.0f;
        while(distanceCheck < distance)
        {

            if(rayLength.x < rayLength.y)
            {
                currentTilePos.x += tileStep.x;
                distanceCheck = rayLength.x;
                rayLength.x += rayStep.x;
            }
            else
            {
                currentTilePos.y += tileStep.y;
                distanceCheck = rayLength.y;
                rayLength.y += rayStep.y;
            }

            currentTileID = lbmg->GetTile(currentTilePos.x, currentTilePos.y);
            
            if(this->IsWallTile(currentTileID))
            {
                if(distanceCheck < distance)
                {
                    // printf("MinitaurController - Blocked\n");
                    return false;
                }
                // printf("MinitaurController - Length Exceeded\n");
                break;
            }
            if(distanceCheck >= distance)
            {
                // printf("MinitaurController - Length Exceeded\n");
            }
        }
    }
    // printf("MinitaurController - Detected\n");
    return true;
}

bool MinitaurController::IsWallTile(int p_tile_id)
{
    return (p_tile_id >= Tile::WallBottomLeft) && (p_tile_id <= Tile::WallTop);
}

glm::vec2 MinitaurController::GetTileWorldPos(glm::ivec2 p_tile_pos)
{
    glm::vec2 res = glm::vec2(
        (float)p_tile_pos.x * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE), 
        (float)p_tile_pos.y * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE)
    );
    return res;
}