//-----------------------------------------------------------------------------
// File: HarpyController.cpp
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Controls harpy attacks & behaviours.
//-----------------------------------------------------------------------------
#include "HarpyController.h"
#include "PlayerController.h"
#include "AttackDamageComponent.h"
#include "HomingComponent.h"
#include "TimedDestroyerComponent.h"
#include "DDACalculator.h"

// !-- Aurora added this --!
#include "inventory/ItemDropCreator.h"
#include <LightComponent.h>

#include <math.h>
#include <cassert>

HarpyController::HarpyController()
{
    wolf::EventManager::AddListener<InfightingEvent, HarpyController, &HarpyController::HandleInfighting>(*this);
}  

HarpyController::~HarpyController()
{
    wolf::EventManager::RemoveListener<InfightingEvent, HarpyController, &HarpyController::HandleInfighting>(*this);
    g_blackboard.UnregisterEnemy(GetGameObject()->GetID());

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
    m_rangedWindupTime = data.rangedWindup;
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

    // Init attack state members
    m_rangedWindupTimer = m_rangedWindupTime;
    m_attackChain = 0;

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

    // Register with blackboard
    g_blackboard.RegisterEnemy(GetGameObject()->GetID());
    
     // Setup combat behavior tree
     SetupCombatBehaviorTree();

}


void HarpyController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_active || !m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    if (m_pTarget)
    {
        auto* targetHealth = m_pTarget->GetComponent<HealthComponent>();
        if (!targetHealth || targetHealth->GetHealth() <= 0) 
        {
            // wolf::Warning("BLUD CAN'T FIND A TARGET");
            RevertBackToPlayer();
        }
    }
    
    // Update the base class
    EnemyController::Update(delta);
    
    // Check if minitaur is petrified
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::DEATH);
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

    // Optimize AI updates - only run on a timer
    static std::map<int, float> s_aiUpdateTimers;
    int myID = GetGameObject()->GetID();
    
    if (s_aiUpdateTimers.find(myID) == s_aiUpdateTimers.end()) {
        s_aiUpdateTimers[myID] = 0.0f;
    }
    
    s_aiUpdateTimers[myID] += delta;
    
    // Run expensive AI operations at most every 200ms
    bool shouldUpdateAI = (s_aiUpdateTimers[myID] > 0.2f);
    if (shouldUpdateAI) {
        // Evaluate strategy
        EvaluateStrategy();
        
        // Update blackboard
        UpdateBlackboard();
        
        // Reset timer
        s_aiUpdateTimers[myID] = 0.0f;
    }
    
    // Update behavior tree
    if (m_combatBehaviorTree && (m_state == EnemyState::CHASING || m_state == EnemyState::IDLE || m_state == EnemyState::ATTACKING)) {
        m_combatBehaviorTree->Update(delta, &g_blackboard);
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
        case EnemyState::PROSPECT:
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
    m_pAnimComponent->SetLayer(11);
    m_pAnimComponent->SetLightingEnabled(false);
}

void HarpyController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform) return;

    // Calculate the direction towards the player
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    // Calculate direction vector
    glm::vec2 direction;
    if (distanceToPlayer > 0.01f) {
        direction = (targetPosition - currentPosition) / distanceToPlayer; // Normalized direction
    } else {
        direction = glm::vec2(0.0f);
    }

    // Get current strategy to determine preferred distance
    Strategy currentStrategy = g_blackboard.GetStrategy();
    float preferredDistance = m_rangedRange * 0.9f; // Default position
    
    if (currentStrategy == Strategy::DEFENSIVE) {
        preferredDistance = m_rangedRange * 0.95f; // Stay further back when defensive
    } else if (currentStrategy == Strategy::AGGRESSIVE) {
        preferredDistance = m_rangedRange * 0.7f; // Closer when aggressive
    }
    
    // Apply hysteresis - only adjust position if we're significantly off from desired range
    const float hysteresisRange = 20.0f; // Distance buffer zone
    
    // Define a scale factor based on how far from the preferred distance we are
    float speedScale = 1.0f;
    
    // If we're close to preferred distance, slow down significantly to prevent oscillation
    if (std::abs(distanceToPlayer - preferredDistance) < hysteresisRange) {
        speedScale = 0.3f * std::abs(distanceToPlayer - preferredDistance) / hysteresisRange;
        
        // If very close to the target distance, just stop
        if (std::abs(distanceToPlayer - preferredDistance) < 5.0f) {
            m_pVelocity->SetVelocity(glm::vec2(0.0f));
            return;
        }
    }
    
    // Adjust direction based on whether we need to move toward or away from player
    if (distanceToPlayer > preferredDistance + hysteresisRange) {
        // Need to move closer
        m_pVelocity->SetVelocity(direction * m_chaseSpeed * speedScale);
    } 
    else if (distanceToPlayer < preferredDistance - hysteresisRange) {
        // Need to move away
        m_pVelocity->SetVelocity(-direction * m_chaseSpeed * speedScale);
    }
    else {
        // Apply velocity smoothing - blend new velocity with current
        glm::vec2 currentVel = m_pVelocity->GetVelocity();
        
        // Gradually reduce velocity when in the desired range
        m_pVelocity->SetVelocity(currentVel * 0.9f);
        
        // If velocity is very small, just stop completely
        if (glm::length(m_pVelocity->GetVelocity()) < 5.0f) {
            m_pVelocity->SetVelocity(glm::vec2(0.0f));
        }
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

//-----------------------------------------------------------------------------
// HandleChasingState - Modified to integrate with behavior tree
//-----------------------------------------------------------------------------
void HarpyController::HandleChasingState(float delta) {
    // If we're repositioning, don't interfere with that movement
    if (m_isRepositioning) {
        MoveToOptimalPosition(delta);
        return;
    }

    // Track player distance
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);
    
    // Handle movement with the improved MoveTowardsTarget method
    MoveTowardsTarget(delta);
    
    // Attack if in range and cooldown complete
    if (distanceToPlayer <= m_rangedRange && m_rangedTimer <= 0.0f && 
        m_transitionTimer.Elapsed() >= m_transitionDelay) {
        ChangeState(EnemyState::ATTACKING);
    }
}



void HarpyController::HandleAttackingState(float delta) {
    if (!m_pTarget) {
        ChangeState(EnemyState::IDLE);
        return;
    }

    // If winding up attack
    if(m_rangedWindupTimer > 0.0f) {
        m_rangedWindupTimer -= delta;

        // Visual feedback - glow effect
        glm::vec3 currentTint = m_pAnimComponent->GetTint();
        glm::vec3 nextTint = currentTint + glm::vec3(delta / (m_rangedWindupTime * 0.5f));
        m_pAnimComponent->SetTint(nextTint);
        
        // Adjust windup time based on attack pattern
        float windupMultiplier = 1.0f;
        switch (m_currentAttackPattern) {
            case AttackPattern::SINGLE: windupMultiplier = 1.2f; break; // Longer windup for powerful shot
            case AttackPattern::SPREAD: windupMultiplier = 0.9f; break; // Standard windup
            case AttackPattern::BURST: windupMultiplier = 0.7f; break;  // Faster windup for burst
        }
        
        // If player moves far away during windup, we might want to cancel or adjust
        const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
        const float distanceToPlayer = glm::length(targetPosition - currentPosition);
        
        if (distanceToPlayer > m_rangedRange * 1.5f) {
            // Target moved too far - cancel attack
            m_pAnimComponent->SetTint(glm::vec3(1.0f)); // Reset tint
            ChangeState(EnemyState::CHASING);
            return;
        }
    }
    // Attack is ready to fire
    else {
        m_pAnimComponent->SetTint(glm::vec3(1.0f)); // Reset windup tint
        
        // Perform attack based on selected pattern
        switch (m_currentAttackPattern) {
            case AttackPattern::SINGLE:
                PerformSingleShot();
                break;
                
            case AttackPattern::SPREAD:
                PerformSpreadShot();
                break;
                
            case AttackPattern::BURST:
                PerformBurstAttack();
                break;
        }
        
        // Handle attack chain continuation or exit
        if (m_attackChain > 0) {
            // Reduce windup time for follow-up attacks
            m_rangedWindupTimer = m_rangedWindupTime * 0.6f;
            ChangeState(EnemyState::ATTACKING);
        } else {
            // End attack sequence
            ChangeState(EnemyState::CHASING);
        }
        return;
    }
}



void HarpyController::HandlePetrifiedState(float delta)
{
    ChangeState(EnemyState::DEATH);
}

void HarpyController::HandleStunnedState(float delta)
{
    if(m_stunnedTimer >= m_stunnedTime)
    {
        if
        (m_last_state == EnemyState::IDLE)
        {
            SetEmote(EnemyEmote::EXCLAMATION);   
        }
        ChangeState(EnemyState::CHASING);
        return;
    }
    else
    {
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
        m_stunnedTimer += delta;
    }
}

void HarpyController::UpdateAnimationBasedOnDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName;

    // Get the current velocity to determine direction
    glm::vec2 velocity = m_pVelocity->GetVelocity();
    
    // Apply smoothing to the velocity vector for animation purposes only
    m_smoothedVelocity = m_smoothedVelocity * (1.0f - ANIMATION_SMOOTHING_FACTOR) + 
                         velocity * ANIMATION_SMOOTHING_FACTOR;
    
    // Only update animation if the Harpy is moving
    if (glm::length(m_smoothedVelocity) > 0.01f)  // Ensure the velocity is not zero
    {
        // Check if the movement is more along the X or Y axis
        if (fabs(m_smoothedVelocity.x) > fabs(m_smoothedVelocity.y))
        {
            // Moving left or right
            animationName = (m_smoothedVelocity.x > 0.0f) ? "StandEast" : "StandWest";
        }
        else
        {
            // Moving up or down
            animationName = (m_smoothedVelocity.y > 0.0f) ? "StandNorth" : "StandSouth";
        }
    }
    else
    {
        // If not moving, keep the last animation
        return;
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
                ColliderComponent* pItemCollider = pItem->GetComponent<ColliderComponent>();
                if (pItemCollider)
                {
                    // Disable the collider after knockback
                    pItemCollider->SetActive(false);
                }
            }

            // Then we delete the harpy
            GetGameObject()->Delete();
        }
        m_lieDeadTimer += delta;
    }  
}

void HarpyController::EnterAttackState() {
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
    
    // Set attack chain if not already set
    if (m_attackChain <= 0) {
        // Default chain logic
        if (m_currentAttackPattern == AttackPattern::BURST) {
            m_attackChain = 3; // Burst always fires 3 volleys
        } else {
            // For other patterns, randomize between 1-2 shots
            m_attackChain = m_RNG.NextInt(1, 2);
        }
    }
    
    // Set windup timer based on pattern
    switch (m_currentAttackPattern) {
        case AttackPattern::SINGLE:
            m_rangedWindupTimer = m_rangedWindupTime * 1.2f; // Longer for powerful shot
            break;
            
        case AttackPattern::SPREAD:
            m_rangedWindupTimer = m_rangedWindupTime;
            break;
            
        case AttackPattern::BURST:
            m_rangedWindupTimer = m_rangedWindupTime * 0.7f; // Faster for burst
            break;
    }
    
    // Visual feedback for attack type
    if (m_pAnimComponent) {
        switch (m_currentAttackPattern) {
            case AttackPattern::SINGLE:
                m_pAnimComponent->SetTint(glm::vec3(1.0f, 0.5f, 0.0f)); // Orange tint for single shot
                break;
                
            case AttackPattern::SPREAD:
                m_pAnimComponent->SetTint(glm::vec3(0.8f, 0.0f, 0.8f)); // Purple tint for spread
                break;
                
            case AttackPattern::BURST:
                m_pAnimComponent->SetTint(glm::vec3(1.0f, 0.0f, 0.0f)); // Red tint for burst
                break;
        }
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

void HarpyController::EnterDeathState()
{
    SetEmote(EnemyEmote::NONE);
}

void HarpyController::ExitAttackState()
{
    if(m_pAnimComponent != nullptr)
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }
    
    m_rangedTimer = m_rangedCooldown;
    m_rangedWindupTimer = m_rangedWindupTime;
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
    // std::cout << "HarpyController - p_emote: " << p_emote << std::endl;
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

void HarpyController::HandleInfighting(const InfightingEvent& event)
{
    if (event.m_pVictim == GetGameObject()) // This Harpy got hit
    {
            // Ignore if already attacking this enemy
            if (m_pTarget == event.m_pAttacker) return;

            // **Switch target to the attacker and start fighting back**
            m_pTarget = event.m_pAttacker;
            ChangeState(EnemyState::CHASING);

            // wolf::Log("Harpy " + std::to_string(GetGameObject()->GetID()) + 
            //                 " is now fighting " + std::to_string(m_pTarget->GetID()));
    }
}

void HarpyController::RevertBackToPlayer()
{
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        // wolf::Warning("LIL BLUD CAN'T FIND A TARGET, SO HE'S SWITCHING BACK TO THE PLAYER");
        return;
    }

    // If no player found, log a warning
    // wolf::Warning("BLUD CAN'T FIND A TARGET");
    m_pTarget = nullptr; // No valid target
}

void HarpyController::SetupCombatBehaviorTree() {
    // Create the root selector
    auto root = std::make_unique<Selector>();
    
    // === REPOSITIONING SEQUENCE ===
    auto repositioningSequence = std::make_unique<Sequence>();
    
    // Condition: Check if we need to reposition
    repositioningSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { return ShouldReposition(); }
    ));
    
    // Action: Move to optimal position
    repositioningSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            if (!m_isRepositioning) {
                // Calculate target position
                m_targetPosition = GetOptimalAttackPosition();
                m_isRepositioning = true;
                
                // Visual feedback
                SetEmote(EnemyEmote::QUESTION);
            }
            
            // Move towards target position
            MoveToOptimalPosition(delta);
            
            // Check if we've reached the position
            glm::vec2 currentPos = m_pTransform->GetGlobalPosition();
            if (glm::length(m_targetPosition - currentPos) < 10.0f) {
                m_isRepositioning = false;
                m_repositionTimer = 0.0f;
                return BehaviorNode::Status::SUCCESS;
            }
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add repositioning sequence to root
    root->AddBehaviorNode(std::move(repositioningSequence));
    
    // === ATTACK SEQUENCE ===
    auto attackSequence = std::make_unique<Sequence>();
    
    // Condition: Check if player is in attack range
    attackSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { 
            return GetDistanceToTarget() <= m_rangedRange && m_rangedTimer <= 0.0f;
        }
    ));
    
    // Condition: Check if player is in line of sight
    attackSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { return IsTargetInSight(); }
    ));
    
    // Action: Perform attack based on current pattern
    attackSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            // If not already attacking, start attack
            if (m_state != EnemyState::ATTACKING) {
                ChangeState(EnemyState::ATTACKING);
                
                // Set emote to indicate attack
                SetEmote(EnemyEmote::EXCLAMATION);
                
                // Change attack pattern based on situation
                int nearbyHarpies = g_blackboard.GetInt("nearbyHarpies");
                
                // Choose attack pattern
                if (nearbyHarpies >= 2) {
                    // Coordinate with other harpies based on ID
                    int myID = GetGameObject()->GetID();
                    int patternID = myID % 3;
                    
                    switch (patternID) {
                        case 0: m_currentAttackPattern = AttackPattern::SINGLE; break;
                        case 1: m_currentAttackPattern = AttackPattern::SPREAD; break;
                        case 2: m_currentAttackPattern = AttackPattern::BURST; break;
                    }
                } else {
                    // Solo harpy - randomize attack pattern with weights
                    int randPattern = m_RNG.NextInt(1, 10);
                    if (randPattern <= 5) { // 50% chance
                        m_currentAttackPattern = AttackPattern::SINGLE;
                    } else if (randPattern <= 8) { // 30% chance
                        m_currentAttackPattern = AttackPattern::SPREAD;
                    } else { // 20% chance
                        m_currentAttackPattern = AttackPattern::BURST;
                    }
                }
                
                return BehaviorNode::Status::RUNNING;
            }
            
            // Check if attack is complete
            if (m_state != EnemyState::ATTACKING) {
                return BehaviorNode::Status::SUCCESS;
            }
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add attack sequence to root
    root->AddBehaviorNode(std::move(attackSequence));
    
    // === CHASE SEQUENCE ===
    auto chaseSequence = std::make_unique<Sequence>();
    
    // Condition: Check if player is visible but not in attack range
    chaseSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { 
            return IsTargetInSight() && GetDistanceToTarget() > m_rangedRange;
        }
    ));
    
    // Action: Move towards player
    chaseSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            if (m_state != EnemyState::CHASING) {
                ChangeState(EnemyState::CHASING);
            }
            
            // Standard chase behavior is handled in HandleChasingState
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add chase sequence to root
    root->AddBehaviorNode(std::move(chaseSequence));
    
    // Create behavior tree with root
    m_combatBehaviorTree = std::make_unique<BehaviorTree>(std::move(root));
}

//-----------------------------------------------------------------------------
// EvaluateStrategy - Determine optimal strategy based on game state
//-----------------------------------------------------------------------------
void HarpyController::EvaluateStrategy() {
    if (!m_pTarget || !m_pHealth) return;
    
    // Cache values
    int myID = GetGameObject()->GetID();
    float selfHealth = m_pHealth->GetHealth();
    float healthPercentage = (selfHealth / m_pHealth->GetMaxHealth()) * 100.0f;
    
    // Get player health
    float playerHealth = 100.0f;
    auto* playerHealthComp = m_pTarget->GetComponent<HealthComponent>();
    if (playerHealthComp) {
        playerHealth = playerHealthComp->GetHealth();
    }
    
    // Count nearby harpies
    int nearbyHarpies = g_blackboard.GetInt("nearbyHarpies");
    
    // Strategy selection
    
    // 1. Low health - prioritize distance and safety
    if (healthPercentage < 30.0f) {
        g_blackboard.SetStrategy(Strategy::DEFENSIVE);
        return;
    }
    
    // 2. Player low health - aggressive attacks to finish them
    if (playerHealth < 20.0f) {
        g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
        return;
    }
    
    // 3. Group tactics
    if (nearbyHarpies >= 1) {
        // When in a group, assign varied roles
        int roleSelector = myID % 3;
        switch (roleSelector) {
            case 0:
                g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
                break;
            case 1:
                g_blackboard.SetStrategy(Strategy::FLANKING);
                break;
            case 2:
                g_blackboard.SetStrategy(Strategy::DEFENSIVE);
                break;
        }
    } else {
        // Solo harpy behavior - adaptive based on conditions
        auto* playerController = m_pTarget->GetComponent<PlayerController>();
        bool playerIsAttacking = false;
        
        if (playerController) {
            playerIsAttacking = playerController->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING;
        }
        
        // If player is attacking or charging, keep distance
        if (playerIsAttacking) {
            g_blackboard.SetStrategy(Strategy::DEFENSIVE);
        } else {
            // Otherwise, default to aggressive
            g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
        }
    }
}

//-----------------------------------------------------------------------------
// UpdateBlackboard - Updates shared information in the blackboard
//-----------------------------------------------------------------------------
void HarpyController::UpdateBlackboard() {
    if (!m_pTarget || !m_pTransform) return;
    
    // Calculate and store distance to player
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    float distanceToPlayer = glm::length(targetPos - selfPos);
    
    g_blackboard.SetFloat("playerDistance", distanceToPlayer);
    
    // Update health data
    static float lastHealth = -1.0f;
    if (m_pHealth) {
        float currentHealth = m_pHealth->GetHealth();
        if (std::abs(currentHealth - lastHealth) > 0.1f) {
            g_blackboard.SetFloat("selfHealth", currentHealth);
            g_blackboard.SetFloat("selfHealthPercentage", 
                                (currentHealth / m_pHealth->GetMaxHealth()) * 100.0f);
            lastHealth = currentHealth;
        }
    }
    
    // Count nearby harpies periodically
    static float s_harpyCountTimer = 0.0f;
    static int s_cachedNearbyHarpies = 0;
    
    s_harpyCountTimer += 0.1f; // Approximate delta time
    if (s_harpyCountTimer > 0.5f) {
        s_cachedNearbyHarpies = 0;
        for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<HarpyController>()) {
            if (controller.GetGameObject() != GetGameObject() && controller.GetTarget() == m_pTarget) {
                glm::vec2 enemyPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                float distanceToEnemy = glm::length(enemyPos - selfPos);
                
                if (distanceToEnemy < m_detectionRange * 1.5f) {
                    s_cachedNearbyHarpies++;
                }
            }
        }
        g_blackboard.SetInt("nearbyHarpies", s_cachedNearbyHarpies);
        s_harpyCountTimer = 0.0f;
    }
}



//-----------------------------------------------------------------------------
// IsTargetInSight - Determine if player is visible
//-----------------------------------------------------------------------------
bool HarpyController::IsTargetInSight() {
    if (!m_pTarget) return false;
    
    // Use line-of-sight check here
    // For now, just using distance as a proxy
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);
    
    return distanceToPlayer <= m_detectionRange;
}

//-----------------------------------------------------------------------------
// GetDistanceToTarget - Calculate distance to current target
//-----------------------------------------------------------------------------
float HarpyController::GetDistanceToTarget() const {
    if (!m_pTarget || !m_pTransform) return 99999.0f;
    
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    return glm::length(targetPosition - currentPosition);
}

//-----------------------------------------------------------------------------
// ShouldReposition - Determine if harpy should find a new position
//-----------------------------------------------------------------------------
bool HarpyController::ShouldReposition() {
    if (!m_pTarget || !m_pTransform) return false;
    
    // Don't reposition during certain states
    if (m_state == EnemyState::ATTACKING || 
        m_state == EnemyState::STUNNED || 
        m_state == EnemyState::PETRIFIED ||
        m_state == EnemyState::DEATH ||
        m_isRepositioning) { // Don't start new repositioning if already repositioning
        return false;
    }
    
    // Update reposition timer
    static float s_lastUpdate = 0.0f;
    m_repositionTimer += 0.1f - s_lastUpdate;
    s_lastUpdate = 0.1f;
    
    // Check if it's time to consider repositioning (less frequent checks)
    if (m_repositionTimer < m_repositionDelay) {
        return false;
    }
    
    // Reset the timer regardless of whether we reposition
    m_repositionTimer = 0.0f;
    
    // If we're defensive, reposition more frequently
    Strategy currentStrategy = g_blackboard.GetStrategy();
    if (currentStrategy == Strategy::DEFENSIVE) {
        m_repositionDelay = 5.0f; // Less frequent repositioning (was 3.0f)
    } else {
        m_repositionDelay = 8.0f; // Even less frequent (was 5.0f)
    }
    
    // Only reposition if we have a very good reason (random chance + positioning factors)
    // This greatly reduces how often harpies reposition
    if (m_RNG.NextFloat(0.0f, 1.0f) > 0.3f) { // Only 30% chance to even consider repositioning
        return false;
    }
    
    // More criteria before actually repositioning
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);
    
    // If we're already at a good distance, don't reposition
    Strategy strategy = g_blackboard.GetStrategy();
    float idealDistance = m_rangedRange * 0.9f;
    
    if (strategy == Strategy::DEFENSIVE) idealDistance = m_rangedRange * 0.95f;
    else if (strategy == Strategy::AGGRESSIVE) idealDistance = m_rangedRange * 0.7f;
    
    // If already near ideal distance, no need to reposition
    if (std::abs(distanceToPlayer - idealDistance) < 30.0f) {
        return false;
    }
    
    return true; // Much less frequent repositioning
}

//-----------------------------------------------------------------------------
// GetOptimalAttackPosition - Find best position for harpy to attack from
//-----------------------------------------------------------------------------
glm::vec2 HarpyController::GetOptimalAttackPosition() {
    if (!m_pTarget || !m_pTransform) {
        return m_pTransform->GetGlobalPosition();
    }
    
    glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    
    // Determine optimal attack distance based on strategy
    Strategy currentStrategy = g_blackboard.GetStrategy();
    float optimalDistance = m_rangedRange * 0.9f; // Default: just inside attack range
    
    if (currentStrategy == Strategy::DEFENSIVE) {
        optimalDistance = m_rangedRange * 0.95f; // Further when defensive
    } else if (currentStrategy == Strategy::AGGRESSIVE) {
        optimalDistance = m_rangedRange * 0.7f; // Closer when aggressive
    }
    
    // For group coordination, distribute positions around player
    int nearbyHarpies = g_blackboard.GetInt("nearbyHarpies");
    int myID = GetGameObject()->GetID();
    
    // Generate potential positions in a circle around player
    std::vector<glm::vec2> potentialPositions;
    int angleCount = 8; // Check 8 evenly distributed directions
    
    for (int i = 0; i < angleCount; i++) {
        float angle = (i * 2.0f * glm::pi<float>()) / angleCount;
        glm::vec2 direction(cos(angle), sin(angle));
        glm::vec2 position = playerPos + direction * optimalDistance;
        potentialPositions.push_back(position);
    }
    
    // Filter and score positions
    std::vector<std::pair<glm::vec2, float>> scoredPositions;
    
    // Get DDACalculator for wall checking
    DDACalculator* pDDA = DDACalculator::GetInstance();
    
    for (const auto& pos : potentialPositions) {
        // First use DDACalculator to check if this position is in a wall
        if (pDDA && pDDA->GetLabyrinthManager()) {
            LabyrinthManager* pLBMG = pDDA->GetLabyrinthManager();
            glm::ivec2 tilePos = pLBMG->GetTilePosition(pos);
            int tileID = pLBMG->GetTile(tilePos.x, tilePos.y);
            
            // Skip this position if it's in a wall tile
            if (tileID >= Tile::WallBottomLeft && tileID <= Tile::WallTop) {
                continue;
            }
        }
        
        // Position is not in a wall, continue with other checks
        if (IsPositionSafe(pos)) {
            float score = 0.0f;
            
            // Distance from current position (closer is better)
            float distanceFromCurrent = glm::length(pos - selfPos);
            score -= distanceFromCurrent * 0.1f;
            
            // Check if position avoids clustering with other harpies
            bool isClear = true;
            for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<HarpyController>()) {
                if (controller.GetGameObject()->GetID() != myID) {
                    glm::vec2 otherPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    float distToOther = glm::length(otherPos - pos);
                    
                    if (distToOther < 50.0f) {
                        isClear = false;
                        break;
                    }
                }
            }
            
            if (isClear) {
                score += 30.0f;
            }
            
            // Prefer positions aligned with group strategy
            if (currentStrategy == Strategy::FLANKING) {
                // When flanking, prefer positions not in player's field of view
                auto* playerController = m_pTarget->GetComponent<PlayerController>();
                if (playerController) {
                    glm::vec2 playerFacing = playerController->GetLastFacingDirectionVector();
                    glm::vec2 toPosition = glm::normalize(pos - playerPos);
                    float dotProduct = glm::dot(playerFacing, toPosition);
                    
                    // Negative dot product means we're behind player
                    if (dotProduct < 0) {
                        score += 25.0f;
                    }
                }
            }
            
            scoredPositions.push_back({pos, score});
        }
    }
    
    // If no valid positions found, return current position
    if (scoredPositions.empty()) {
        return selfPos;
    }
    
    // Find position with highest score
    std::sort(scoredPositions.begin(), scoredPositions.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return scoredPositions[0].first;
}

//-----------------------------------------------------------------------------
// MoveToOptimalPosition - Move harpy to target position
//-----------------------------------------------------------------------------
void HarpyController::MoveToOptimalPosition(float delta) {
    if (!m_pVelocity || !m_pTransform) return;
    
    glm::vec2 currentPos = m_pTransform->GetGlobalPosition();
    glm::vec2 toTarget = m_targetPosition - currentPos;
    float distanceToTarget = glm::length(toTarget);
    
    // Only move if we're not very close to the target position
    if (distanceToTarget > 10.0f) {
        glm::vec2 direction = toTarget / distanceToTarget; // Normalized direction
        
        // Apply smoothing - current velocity contributes 70%, new direction 30%
        glm::vec2 currentVelocity = m_pVelocity->GetVelocity();
        glm::vec2 targetVelocity = direction * m_chaseSpeed * 0.8f;
        
        // Blend velocities for smooth movement
        glm::vec2 blendedVelocity = currentVelocity * 0.7f + targetVelocity * 0.3f;
        
        // Scale down movement as we get closer to the target
        float slowdownFactor = std::min(1.0f, distanceToTarget / 50.0f);
        
        m_pVelocity->SetVelocity(blendedVelocity * slowdownFactor);
    } else {
        // We've arrived at the target position
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

//-----------------------------------------------------------------------------
// IsPositionSafe - Check if a position is valid for the harpy
//-----------------------------------------------------------------------------
bool HarpyController::IsPositionSafe(const glm::vec2& position) {
    return true;
}

//-----------------------------------------------------------------------------
// PerformSingleShot - Fires a single focused projectile
//-----------------------------------------------------------------------------
void HarpyController::PerformSingleShot() {
    if (!m_pTarget) return;
    
    auto& scene = this->GetGameObject()->GetScene();
    
    // Get positions
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 direction = targetPosition - currentPosition;
    
    // Normalize direction
    if (glm::length(direction) > 0.01f) {
        direction = glm::normalize(direction);
    } else {
        direction = glm::vec2(0.0f, -1.0f); // Default direction
    }
    
    // Create a single powerful projectile
    auto& projectile = scene.CreateObject2D();
    
    // Set status effects (single shot has longer burning)
    std::vector<std::pair<StatusComponent::StatusEffectType, float>> statusEffects = 
    {
        std::pair<StatusComponent::StatusEffectType, float>(StatusComponent::StatusEffectType::BURNING, 7.0f)
    };
    
    // Higher damage for focused shot
    auto& attackDamageComponent = projectile.AddComponent<AttackDamageComponent>(
        m_baseDamage * 1.5f, m_pColliderManager, 0.0f, statusEffects, GetGameObject());
    
    auto& projectileSprite = projectile.AddComponent<AnimatedSprite2D>("data/fireball_anim_init.yaml");
    projectileSprite.SetLayer(20);
    
    // Larger hitbox
    glm::vec2 projectileDimensions = glm::vec2(10.0f, 10.0f);
    glm::vec2 hurtboxOffset = glm::vec2(-5.0f, -5.0f);
    auto& projectileCollider = projectile.AddComponent<ColliderComponent>(
        ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
    projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
    projectileCollider.SetIgnoreTag(this->GetGameObject()->GetID());
    
    
    // Better tracking
    auto& projectileHoming = projectile.AddComponent<HomingComponent>(m_pTarget, 8.0f, 0.2f);
    auto& projectileTimedDestroyer = projectile.AddComponent<TimedDestroyerComponent>(10);
    
    // Faster velocity
    auto& projectileVelocityComponent = projectile.AddComponent<VelocityComponent>();
    projectileVelocityComponent.SetVelocity(direction * 200.0f);
    
    // Position and scale
    projectile.GetComponent<wolf::Transform2D>()->SetPosition(m_pTransform->GetGlobalPosition());
    projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(4.0f));

    // Add a light to the projectile (Aurora added this)
    wolf::GameObject* pLightGO = &scene.CreateObject2D();
    auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(1.0f, 0.64f, 0.0f, 0.75f), 50.0f, true);
    projectile.AddChild(*pLightGO);
    pLightComponent.Init();
}

//-----------------------------------------------------------------------------
// PerformSpreadShot - Fires multiple projectiles in a spread pattern
//-----------------------------------------------------------------------------
void HarpyController::PerformSpreadShot() {
    if (!m_pTarget) return;
    
    auto& scene = this->GetGameObject()->GetScene();
    
    // Get positions
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 direction = targetPosition - currentPosition;
    
    // Normalize direction
    if (glm::length(direction) > 0.01f) {
        direction = glm::normalize(direction);
    } else {
        direction = glm::vec2(0.0f, -1.0f); // Default direction
    }
    
    // Calculate perpendicular vector for spread
    glm::vec2 perpendicular = glm::vec2(-direction.y, direction.x);
    
    // Create 5 projectiles in a spread pattern
    int numProjectiles = 5;
    float spreadWidth = 40.0f;
    
    for (int i = 0; i < numProjectiles; i++) {
        auto& projectile = scene.CreateObject2D();
        
        // Set status effects
        std::vector<std::pair<StatusComponent::StatusEffectType, float>> statusEffects = 
        {
            std::pair<StatusComponent::StatusEffectType, float>(StatusComponent::StatusEffectType::BURNING, 3.0f)
        };
        
        // Lower damage for spread shots
        auto& attackDamageComponent = projectile.AddComponent<AttackDamageComponent>(
            m_baseDamage * 0.8f, m_pColliderManager, 0.0f, statusEffects, GetGameObject());
        
        auto& projectileSprite = projectile.AddComponent<AnimatedSprite2D>("data/fireball_anim_init.yaml");
        projectileSprite.SetLayer(20);
        
        // Standard hitbox
        glm::vec2 projectileDimensions = glm::vec2(10.0f, 10.0f);
        glm::vec2 hurtboxOffset = glm::vec2(-5.0f, 5.0f);
        auto& projectileCollider = projectile.AddComponent<ColliderComponent>(
            ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
        projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
        projectileCollider.SetIgnoreTag(this->GetGameObject()->GetID());
        
        // Less tracking for spread shots
        auto& projectileHoming = projectile.AddComponent<HomingComponent>(m_pTarget, 2.0f, 0.05f);
        auto& projectileTimedDestroyer = projectile.AddComponent<TimedDestroyerComponent>(8);
        
        // Calculate spread offset
        float spreadOffset = (i - (numProjectiles - 1) / 2.0f) * (spreadWidth / (numProjectiles - 1));
        glm::vec2 offset = perpendicular * spreadOffset;
        
        // Apply slight angle variation to direction
        float angleVariation = spreadOffset / spreadWidth * 0.3f; // Max 30% angle variation
        glm::vec2 adjustedDirection = direction + perpendicular * angleVariation;
        adjustedDirection = glm::normalize(adjustedDirection);
        
        // Set velocity
        auto& projectileVelocityComponent = projectile.AddComponent<VelocityComponent>();
        projectileVelocityComponent.SetVelocity(adjustedDirection * 168.0f);
        
        // Position and scale
        projectile.GetComponent<wolf::Transform2D>()->SetPosition(m_pTransform->GetGlobalPosition() + offset * 0.5f);
        projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(2.5f));

        // Add a light to the projectile (Aurora added this)
        wolf::GameObject* pLightGO = &scene.CreateObject2D();
        auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(1.0f, 0.64f, 0.0f, 0.75f), 50.0f, true);
        projectile.AddChild(*pLightGO);
        pLightComponent.Init();
    }
}

//-----------------------------------------------------------------------------
// PerformBurstAttack - Fires a quick succession of 3 volleys
//-----------------------------------------------------------------------------
void HarpyController::PerformBurstAttack() {
    if (!m_pTarget) return;
    
    // Set attack chain to fire 3 separate volleys
    m_attackChain = 3;
    
    auto& scene = this->GetGameObject()->GetScene();
    
    // Get positions
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 harpyDirection = targetPosition - currentPosition;
    
    // Normalize direction
    if (glm::length(harpyDirection) > 0.01f) {
        harpyDirection = glm::normalize(harpyDirection);
    } else {
        harpyDirection = glm::vec2(0.0f, -1.0f); // Default direction
    }
    
    // Calculate perpendicular for spread
    glm::vec2 perpendicularVector = glm::vec2(-harpyDirection.y, harpyDirection.x);
    
    // Project dimensions
    glm::vec2 projectileDimensions = glm::vec2(10.0f, 10.0f);
    glm::vec2 hurtboxOffset = glm::vec2(-5.0f, 5.0f);
    
    // Default velocity
    glm::vec2 projectileDefaultVelocity = harpyDirection * 168.0f;
    
    // Spawn 3 projectiles per volley
    for(int i = -1; i <= 1; i += 1) {
        auto& projectile = scene.CreateObject2D();
        
        // Status effects
        std::vector<std::pair<StatusComponent::StatusEffectType, float>> statusEffects = 
        {
            std::pair<StatusComponent::StatusEffectType, float>(StatusComponent::StatusEffectType::BURNING, 4.0f)
        };
        
        // Components
        auto& attackDamageComponent = projectile.AddComponent<AttackDamageComponent>(
            m_baseDamage, m_pColliderManager, 0.0f, statusEffects, GetGameObject());
        
        auto& projectileSprite = projectile.AddComponent<AnimatedSprite2D>("data/fireball_anim_init.yaml");
        projectileSprite.SetLayer(20);
        
        auto& projectileCollider = projectile.AddComponent<ColliderComponent>(
            ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
        projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
        projectileCollider.SetIgnoreTag(this->GetGameObject()->GetID());
        
        auto& projectileHoming = projectile.AddComponent<HomingComponent>(m_pTarget, 6.0f, 0.1f);
        auto& projectileTimedDestroyer = projectile.AddComponent<TimedDestroyerComponent>(10);
        
        auto& projectileVelocityComponent = projectile.AddComponent<VelocityComponent>();
        projectileVelocityComponent.SetVelocity(projectileDefaultVelocity);
        
        // Position with spread
        glm::vec2 offset = perpendicularVector * (30.0f * i);
        projectile.GetComponent<wolf::Transform2D>()->SetPosition(m_pTransform->GetGlobalPosition() + offset);
        projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));

        // Add a light to the projectile (Aurora added this)
        wolf::GameObject* pLightGO = &scene.CreateObject2D();
        auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(1.0f, 0.64f, 0.0f, 0.75f), 50.0f, true);
        projectile.AddChild(*pLightGO);
        pLightComponent.Init();
    }
}

