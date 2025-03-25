//-----------------------------------------------------------------------------
// File: MinitaurController.cpp
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Controls minitaur attacks & behaviours.
//-----------------------------------------------------------------------------
#include "MinitaurController.h"
#include "PlayerController.h"
#include "LabyrinthManager.h"
#include "../GLShapesRenderer.h"
#include "../DDACalculator.h"

#include <cassert>

// !- Aurora added this --!
#include "inventory/ItemDropCreator.h"

MinitaurController::MinitaurController()
{
    wolf::EventManager::AddListener<InfightingEvent, MinitaurController, &MinitaurController::HandleInfighting>(*this);
}

MinitaurController::~MinitaurController()
{
    wolf::EventManager::AddListener<InfightingEvent, MinitaurController, &MinitaurController::HandleInfighting>(*this);
    g_blackboard.UnregisterEnemy(GetGameObject()->GetID());
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
    m_meleeWindupTime = data.meleeWindup;
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

    // Init attack state members
    m_meleeWindupTimer = m_meleeWindupTime;

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

    // Find the PathfindingManager in the scene
    bool pathfindingManagerFound = false;
    for (auto&& [entity, pathfindingManager] : GetGameObject()->GetScene().Each<PathfindingManager>())
    {
        m_pPathfindingManager = &pathfindingManager;
        pathfindingManagerFound = true;
        break; // there's only one PathfindingManager in the scene
    }

    if (!pathfindingManagerFound)
    {
        wolf::Warning("MinitaurController: No PathfindingManager found in the scene!");
    }

    g_blackboard.RegisterEnemy(GetGameObject()->GetID());
    
    // Setup combat behavior tree
    SetupCombatBehaviorTree();
}


void MinitaurController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_active || !m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    if (m_pTarget)
    {
        auto* targetHealth = m_pTarget->GetComponent<HealthComponent>();
        if (!targetHealth || targetHealth->GetHealth() <= 0) 
        {
            // wolf::Warning("LIL BLUD CAN'T FIND A TARGET, SO HE'S SWITCHING BACK TO THE PLAYER");
            RevertToPlayerTarget();
        }
    }

    // Update the base class
    EnemyController::Update(delta);

    // Optimize blackboard update - Only update global attack lock once per frame
    // We can make this static to ensure it only happens once per frame
    static float s_lastFrameTime = 0.0f;
    static float s_currentTime = 0.0f;
    s_currentTime += delta;
    if (s_currentTime - s_lastFrameTime > 0.1f) // Only update every 100ms
    {
        int attackingID = g_blackboard.GetAttackingEnemyID();
        if (attackingID != -1) {
            float attackLockTime = g_blackboard.GetFloat("attackLockTime");
            attackLockTime += (s_currentTime - s_lastFrameTime);
            g_blackboard.SetFloat("attackLockTime", attackLockTime);
            
            if (attackLockTime > 2.0f) {
                g_blackboard.SetAttackingEnemyID(-1);
                g_blackboard.SetFloat("attackLockTime", 0.0f);
            }
        }
        s_lastFrameTime = s_currentTime;
    }

    // Optimize AI updates - only run on a timer instead of every frame
    static std::map<int, float> s_aiUpdateTimers;
    int myID = GetGameObject()->GetID();
    
    if (s_aiUpdateTimers.find(myID) == s_aiUpdateTimers.end()) {
        s_aiUpdateTimers[myID] = 0.0f;
    }
    
    s_aiUpdateTimers[myID] += delta;
    
    // Throttle expensive AI operations to run at most every 200ms
    bool shouldUpdateAI = (s_aiUpdateTimers[myID] > 0.2f);
    if (shouldUpdateAI) {
        // Only evaluate strategy occasionally
        EvaluateStrategy();
        
        // Update blackboard with current game state
        UpdateBlackboard();
        
        // Reset the timer
        s_aiUpdateTimers[myID] = 0.0f;
    }
    
    // Combat behavior tree should run more frequently, but not every frame
    static std::map<int, float> s_behaviorUpdateTimers;
    if (s_behaviorUpdateTimers.find(myID) == s_behaviorUpdateTimers.end()) {
        s_behaviorUpdateTimers[myID] = 0.0f;
    }
    
    s_behaviorUpdateTimers[myID] += delta;
    
    // Run behavior tree at 10 fps instead of every frame
    if (s_behaviorUpdateTimers[myID] > 0.05f && (m_state == EnemyState::CHASING || m_state == EnemyState::ATTACKING)) {
        // More frequent updates for solo Minitaurs
        bool isSolo = (g_blackboard.GetInt("nearbyEnemies") == 0);
        float updateFrequency = isSolo ? 0.05f : 0.1f;
        
        if (s_behaviorUpdateTimers[myID] > updateFrequency) {
            m_combatBehaviorTree->Update(delta, &g_blackboard);
            s_behaviorUpdateTimers[myID] = 0.0f;
        }
    }
    

    // Check if minitaur is petrified
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::PETRIFIED);
    }
    else 
    {
        // On exiting petrified state
        if(m_state == EnemyState::PETRIFIED)
        {
            ChangeState(EnemyState::CHASING);
        }         
    }

    // Check if health is below or equal to 0 and not already dying, transition to the DEATH state
    if (m_pHealth->GetHealth() <= 0 && m_state != EnemyState::DEATH)
    {
        // Switch to the DEATH state if the health is depleted
        ColliderComponent* collider = this->GetGameObject()->GetComponent<ColliderComponent>();
        collider->SetActive(false);
        ChangeState(EnemyState::DEATH);
        
        return;
    }

    if(m_meleeTimer > 0.0f)
    {
        // Cooldown timer for next attack
        m_meleeTimer -= delta;
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
        case EnemyState::DODGE:
            HandleDodgeState(delta);
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

    if (m_state != EnemyState::DODGE && m_state != EnemyState::STUNNED && 
        m_state != EnemyState::DEATH && m_state != EnemyState::PETRIFIED) {
        
        if (!m_isDodging && ShouldDodgePlayerAttack()) {
            ChangeState(EnemyState::DODGE);
        }
    }

}
void MinitaurController::EnterDodgeState()
{
    // Initialize dodge state
    m_isDodging = false; // Will be set to true in HandleDodgeState
}

void MinitaurController::ExitDodgeState()
{
    m_isDodging = false;

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
    m_pAnimComponent->SetLayer(9);
}

void MinitaurController::RenderDebugPath()
{

}

void MinitaurController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform || !m_pPathfindingManager)
        return;

    auto& pathData = m_pPathfindingManager->GetPathData(GetGameObject());

    if (pathData.path.empty())
    {
        FallbackToDistanceChecking();
        return;
    }

    glm::ivec2 nextTile = pathData.path.front();
    glm::vec2 nextTileWorldPos = m_pPathfindingManager->GetLabyrinthManager()->GetWorldPosition(nextTile) + glm::vec2(48.0f, 48.0f);
    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 direction = nextTileWorldPos - currentPosition;

    // Check for other Minitaurs in the way
    glm::vec2 avoidance(0.0f);
    bool needsAvoidance = false;
    int myID = GetGameObject()->GetID();
    
    for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
        if (controller.GetGameObject()->GetID() != myID) {
            glm::vec2 otherPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            float distToOther = glm::length(otherPos - currentPosition);
            
            // Only avoid if close enough
            if (distToOther < 70.0f) {
                // Calculate avoidance vector (away from other Minitaur)
                glm::vec2 awayDir = glm::normalize(currentPosition - otherPos);
                
                // Stronger avoidance for closer Minitaurs
                float avoidStrength = 1.0f - (distToOther / 70.0f);
                avoidance += awayDir * avoidStrength;
                needsAvoidance = true;
            }
        }
    }
    
    if (glm::length(direction) > 0.5f)
    {
        direction = glm::normalize(direction);
        
        // Apply avoidance if needed
        if (needsAvoidance) {
            // Normalize avoidance vector
            avoidance = glm::normalize(avoidance);
            
            // Blend between path direction and avoidance
            // More weight to path direction to maintain overall movement
            direction = glm::normalize(direction * 0.7f + avoidance * 0.3f);
        }
        
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
    }
    else
    {
        // Advance to the next tile
        pathData.path.erase(pathData.path.begin());
    }
}

// Fallback to direct distance checking if pathfinding fails
void MinitaurController::FallbackToDistanceChecking()
{
    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    
    // Check if there are other Minitaurs nearby
    glm::vec2 avoidanceVector(0.0f);
    bool needsAvoidance = false;
    int myID = GetGameObject()->GetID();
    
    // Find all nearby Minitaurs and calculate avoidance
    for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
        if (controller.GetGameObject()->GetID() != myID) {
            glm::vec2 otherPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            float distance = glm::length(otherPos - currentPosition);
            
            // If another Minitaur is very close, avoid it
            if (distance < 80.0f) {
                // Calculate avoid direction (away from other Minitaur)
                glm::vec2 awayDir = glm::normalize(currentPosition - otherPos);
                
                // Stronger avoidance for closer Minitaurs
                float avoidStrength = 1.0f - (distance / 80.0f);
                avoidanceVector += awayDir * avoidStrength * 1.5f; // Amplify the avoidance
                
                needsAvoidance = true;
            }
        }
    }
    
    // Calculate basic direction to target
    glm::vec2 direction = targetPosition - currentPosition;
    
    if (glm::length(direction) > 0.01f)
    {
        direction = glm::normalize(direction);
        
        // Apply avoidance if needed
        if (needsAvoidance) {
            // Add randomness to break symmetrical situations
            glm::vec2 randomOffset(m_RNG.NextFloat(-0.1f, 0.1f), m_RNG.NextFloat(-0.1f, 0.1f));
            avoidanceVector = glm::normalize(avoidanceVector + randomOffset);
            
            // Calculate priority based on ID to prevent deadlocks
            // Lower ID Minitaurs get priority for direct path
            int nearbyLowerIDs = 0;
            for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
                int otherID = controller.GetGameObject()->GetID();
                if (otherID < myID) {
                    glm::vec2 otherPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    float dist = glm::length(otherPos - currentPosition);
                    if (dist < 80.0f) {
                        nearbyLowerIDs++;
                    }
                }
            }
            
            // If many lower IDs nearby, give them priority by moving more to avoid them
            float targetWeight = 0.7f - (nearbyLowerIDs * 0.1f);
            targetWeight = glm::clamp(targetWeight, 0.3f, 0.7f);
            
            // Blend between target direction and avoidance vector
            direction = glm::normalize(direction * targetWeight + avoidanceVector * (1.0f - targetWeight));
        }
        
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
    }
    else
    {
        // We're at the target, just apply avoidance if needed
        if (needsAvoidance) {
            m_pVelocity->SetVelocity(avoidanceVector * m_chaseSpeed * 0.5f);
        } else {
            m_pVelocity->SetVelocity(glm::vec2(0.0f));
        }
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
    // Track how long we've been in chasing state to prevent getting stuck
    static std::map<int, float> chasingTimes;
    int id = GetGameObject()->GetID();
    
    if (chasingTimes.find(id) == chasingTimes.end()) {
        chasingTimes[id] = 0.0f;
    }
    
    chasingTimes[id] += delta;
    
    // Check if we're stuck and need to force a state transition
    bool needsForceStateChange = chasingTimes[id] > 5.0f;
    
    // Normal movement behavior
    MoveTowardsTarget(delta);
    
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);
    
    // If player is out of detection range, emote & switch to prospect
    if (distanceToPlayer > m_detectionRange)
    {
        SetEmote(EnemyEmote::QUESTION);
        ChangeState(EnemyState::PROSPECT);
        chasingTimes[id] = 0.0f;
        return;
    }
    
    // Handling being in attack range
    if (distanceToPlayer <= m_meleeRange)
    {
        // If we're allowed to attack (based on timer or forced state change)
        if ((m_transitionTimer.Elapsed() >= m_transitionDelay && m_meleeTimer <= 0.0f) || needsForceStateChange)
        {
            // Check if we can actually attack based on blackboard
            int attackingID = g_blackboard.GetAttackingEnemyID();
            int myID = GetGameObject()->GetID();
            
            // If no one is attacking or we're the attacker or we need to force a change
            if (attackingID == -1 || attackingID == myID || needsForceStateChange) 
            {
                ChangeState(EnemyState::ATTACKING);
                chasingTimes[id] = 0.0f;
                return;
            }
            // Otherwise, reposition to surround player
            else 
            {
                // Get a good position around the player
                glm::vec2 optimalPos = GetOptimalAttackPosition();
                glm::vec2 moveDir = glm::normalize(optimalPos - currentPosition);
                m_pVelocity->SetVelocity(moveDir * m_chaseSpeed * 0.7f); // Slower when positioning
            }
        }
    }
    // If we need to force state change and got here, that means we couldn't attack
    // Reset the timer to avoid continuous forced changes
    if (needsForceStateChange) {
        chasingTimes[id] = 0.0f;
    }
}

void MinitaurController::HandleAttackingState(float delta)
{
    if (!m_pTarget)
    {
        ChangeState(EnemyState::IDLE);
        return;
    }

    // Track how long we've been in the attack state
    static std::map<int, float> attackTimes;
    int id = GetGameObject()->GetID();
    
    if (attackTimes.find(id) == attackTimes.end()) {
        attackTimes[id] = 0.0f;
    }
    
    attackTimes[id] += delta;
    
    // Get current positions for distance checks
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);
    
    // If winding up attack
    if(m_meleeWindupTimer > 0.0f)
    {
        // Brighten sprite to indicate attack
        if(m_pAnimComponent != nullptr)
        {
            glm::vec3 currentTint = m_pAnimComponent->GetTint();
            glm::vec3 nextTint = currentTint + glm::vec3(delta / (m_meleeWindupTime * 0.5f));
            m_pAnimComponent->SetTint(nextTint);
        }

        m_meleeWindupTimer -= delta;
        
        // If player moved too far during windup, we can adjust slightly to follow
        // But only if we're not too far into the windup (first half)
        float windupProgress = 1.0f - (m_meleeWindupTimer / m_meleeWindupTime);
        
        // If player is far outside of melee range, cancel attack
        if (distanceToPlayer > m_meleeRange * 2.0f) {
            m_pAnimComponent->SetTint(glm::vec3(1.0f)); // Reset windup tint
            attackTimes[id] = 0.0f;
            
            // Release attack turn
            int myID = GetGameObject()->GetID();
            int attackingID = g_blackboard.GetAttackingEnemyID();
            if (attackingID == myID) {
                g_blackboard.SetAttackingEnemyID(-1);
            }
            
            ChangeState(EnemyState::CHASING);
            return;
        }
        
        if (windupProgress < 0.5f) {
            // If player is moving away but still potentially reachable
            if (distanceToPlayer > m_meleeRange && distanceToPlayer < m_meleeRange * 1.5f) {
                // Move slightly to adjust position (slower than chase speed)
                glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
                m_pVelocity->SetVelocity(direction * m_chaseSpeed * 0.4f);
            }
        } else {
            // In the second half of windup, commit fully to the attack
            m_pVelocity->SetVelocity(glm::vec2(0.0f));
        }
    }
    // Else, strike
    else
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f)); // Reset windup tint

        // Determine if target's collider is active and a hurtbox
        auto* pCollider = m_pTarget->GetComponent<ColliderComponent>();
        const bool active = pCollider ? (pCollider->IsActive() && pCollider->IsHurtbox()) : false;

        // Apply damage ONLY if player is within melee range
        // Removed the forceAttack option for landing hits
        if (active && distanceToPlayer <= m_meleeRange)
        {
            // Get damage multiplier from blackboard (default to 1.0 if not set)
            float damageMultiplier = g_blackboard.GetFloat("attackDamageMultiplier");
            if (damageMultiplier <= 0.0f) damageMultiplier = 1.0f;

            // Apply damage to the player
            auto* playerHealth = m_pTarget->GetComponent<HealthComponent>();
            if (playerHealth)
            {
                playerHealth->Damage(m_baseDamage * damageMultiplier);
            }

            // Apply knockback to the player
            auto* playerVelocity = m_pTarget->GetComponent<VelocityComponent>();
            if (playerVelocity)
            {
                // Calculate knockback direction and amplify the push
                glm::vec2 knockbackDirection = glm::normalize(targetPosition - currentPosition);
                
                // Stronger knockback for heavy attacks
                float knockbackStrength = 800.0f * damageMultiplier;
                playerVelocity->ApplyKnockback(knockbackDirection, knockbackStrength);
            }

            // Reset attack timer
            attackTimes[id] = 0.0f;
            
            // Create opportunity for other Minitaurs to attack next
            // 50% chance to continue attacking, 50% chance to let others attack
            if (m_RNG.NextInt(0, 1) == 0) {
                // Release attack turn for others
                int myID = GetGameObject()->GetID();
                int attackingID = g_blackboard.GetAttackingEnemyID();
                if (attackingID == myID) {
                    g_blackboard.SetAttackingEnemyID(-1);
                }
                ChangeState(EnemyState::CHASING);
            } else {
                // Chain another attack
                ChangeState(EnemyState::ATTACKING);
            }
            return;
        }
        else
        {
            // If we've been trying to attack for too long or player is too far, give up
            if (attackTimes[id] > 3.0f || distanceToPlayer > m_meleeRange * 1.5f) {
                // Release attack turn for others
                int myID = GetGameObject()->GetID();
                int attackingID = g_blackboard.GetAttackingEnemyID();
                if (attackingID == myID) {
                    g_blackboard.SetAttackingEnemyID(-1);
                }
                attackTimes[id] = 0.0f;
            }
            
            // Return to chasing if player moves out of range
            ChangeState(EnemyState::CHASING);
            return;
        }
    }
}

void MinitaurController::HandlePetrifiedState(float delta)
{
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
                GetGameObject()->Delete();
            }
            m_lieDeadTimer += delta;

        }  
    }

void MinitaurController::EnterAttackState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
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
void MinitaurController::EnterPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::MULTITEX_PETRIFIED);
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
    if(m_pAnimComponent != nullptr)
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }
    
    // Release the attack turn if we were the attacker
    int attackingID = g_blackboard.GetAttackingEnemyID();
    int myID = GetGameObject()->GetID();
    if (attackingID == myID) {
        g_blackboard.SetAttackingEnemyID(-1);
    }
    
    m_meleeTimer = m_meleeCooldown;
    m_meleeWindupTimer = m_meleeWindupTime;
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
    glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = this->m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 endPos = DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos);
    if(endPos == targetPos)
    {
        return true;
    }
    return false;
}

glm::vec2 MinitaurController::GetTileWorldPos(glm::ivec2 p_tile_pos)
{
    glm::vec2 res = glm::vec2(
        (float)p_tile_pos.x * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE), 
        (float)p_tile_pos.y * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE)
    );
    return res;
}

void MinitaurController::HandleInfighting(const InfightingEvent& event)
{
    if (event.m_pVictim == GetGameObject()) // This Minitaur got hit
    {
            // Ignore if already attacking this enemy
            if (m_pTarget == event.m_pAttacker) return;

            // **Minitaur fights back once hit**
            m_pTarget = event.m_pAttacker;
            ChangeState(EnemyState::CHASING);

            // wolf::Log("Minitaur " + std::to_string(GetGameObject()->GetID()) + 
            //                 " is now fighting " + std::to_string(m_pTarget->GetID()));
    }
}

void MinitaurController::RevertToPlayerTarget()
{
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        return;
    }

    // If no player found, log a warning
    // wolf::Warning("BLUD CAN'T FIND A TARGET");
    m_pTarget = nullptr; // No valid target
}

// Helper function to detect player attacks for defensive strategy
bool MinitaurController::ShouldDodgePlayerAttack() {
    if (!m_pTarget) return false;
    
    // Get the PlayerController
    auto* playerController = m_pTarget->GetComponent<PlayerController>();
    if (!playerController) return false;
    
    // Check if player is attacking
    bool playerIsAttacking = playerController->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING;
    
    if (playerIsAttacking) {
        glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
        
        // Check distance - very close threshold
        float distance = glm::length(playerPos - selfPos);
        if (distance > 80.0f) return false; // Only dodge when very close to player
        
        // Check if player is facing us specifically
        glm::vec2 playerFacing = playerController->GetLastFacingDirectionVector();
        glm::vec2 toMinitaur = glm::normalize(selfPos - playerPos);
        float facingDot = glm::dot(playerFacing, toMinitaur);
        
        // Only dodge if the player is facing directly toward THIS minitaur
        if (facingDot > 0.8f) {
            // Check if this minitaur is the closest one in the player's attack direction
            bool isClosestInDirection = true;
            
            // Check other minitaurs
            for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
                // Skip self comparison
                if (controller.GetGameObject()->GetID() == GetGameObject()->GetID()) 
                    continue;
                
                glm::vec2 otherPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                float otherDistance = glm::length(otherPos - playerPos);
                
                // If other minitaur is closer than us
                if (otherDistance < distance) {
                    // Check if it's also in the player's attack direction
                    glm::vec2 toOther = glm::normalize(otherPos - playerPos);
                    float otherDot = glm::dot(playerFacing, toOther);
                    
                    // If other minitaur is also in attack direction and closer, we shouldn't dodge
                    if (otherDot > 0.7f) {
                        isClosestInDirection = false;
                        break;
                    }
                }
            }
            
            return isClosestInDirection;
        }
    }
    
    return false;
}

void MinitaurController::HandleDodgeState(float delta)
{
    DodgeFromPlayer(delta);
}

void MinitaurController::DodgeFromPlayer(float delta)
{
    // Set up dodge direction if not already dodging
    if (!m_isDodging) {
        m_isDodging = true;
        m_dodgeTimer.Restart();
        
        // Get player direction
        glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
        glm::vec2 dirToPlayer = glm::normalize(playerPos - selfPos);
        
        // Check player weapon type for dodge direction
        auto* playerController = m_pTarget->GetComponent<PlayerController>();
        WeaponItem* pWeapon = nullptr;
        if (playerController) {
            pWeapon = playerController->GetHeldWeapon();
        }
        bool isRanged = pWeapon && pWeapon->GetWeaponType() == WeaponType::BOW;
        
        if (isRanged) {
            // For ranged attacks, dodge perpendicular (sideways)
            float rotAngle = m_RNG.FlipCoin() ? 90.0f : -90.0f;
            glm::mat4 rotation = glm::rotate(glm::radians(rotAngle), glm::vec3(0, 0, 1));
            m_dodgeDir = glm::vec2(rotation * glm::vec4(dirToPlayer.x, dirToPlayer.y, 0, 1));
        } else {
            // For melee attacks, dodge away from player
            m_dodgeDir = -dirToPlayer;
        }
        
        // Apply a milder dodge force (reduced from 700.0f)
        m_pVelocity->ApplyKnockback(m_dodgeDir, 400.0f);
        
        // Still keep the emote for visual feedback
        SetEmote(EnemyEmote::EXCLAMATION);
    }
    
    if (m_dodgeTimer.Elapsed() < 0.4f) {
        // Reduced speed multiplier from 1.4f to 1.1f
        m_pVelocity->SetVelocity(m_dodgeDir * m_chaseSpeed * 1.1f);
    } else {
        // End dodge after timer expires
        m_isDodging = false;

        
        // Reset to chasing after dodge
        ChangeState(EnemyState::CHASING);
    }
}


// Helper function to calculate the optimal flanking position
glm::vec2 MinitaurController::CalculateFlankingPosition() {
    if (!m_pTarget || !m_pTransform || !m_pPathfindingManager) {
        return m_pTransform->GetGlobalPosition(); // Fallback to current position
    }
    
    auto* labManager = m_pPathfindingManager->GetLabyrinthManager();
    if (!labManager) {
        return m_pTransform->GetGlobalPosition();
    }
    
    // Get positions
    glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    
    // Try to determine player's facing direction
    auto* playerController = m_pTarget->GetComponent<PlayerController>();
    glm::vec2 playerFacingDir(0.0f);
    
    if (playerController) {
        // Get player's facing direction
        playerFacingDir = playerController->GetLastFacingDirectionVector();
    } else {
        // Fallback - assume player is facing us
        playerFacingDir = glm::normalize(selfPos - playerPos);
    }
    
    // Calculate potential flanking positions:
    // 1. Behind player (opposite to their facing direction)
    // 2. Left side of player (perpendicular to facing)
    // 3. Right side of player (perpendicular to facing)
    
    glm::vec2 behindDir = -playerFacingDir;
    glm::vec2 leftDir = glm::vec2(-playerFacingDir.y, playerFacingDir.x);
    glm::vec2 rightDir = glm::vec2(playerFacingDir.y, -playerFacingDir.x);
    
    float flankDistance = m_meleeRange * 1.2f; // Position just outside melee range
    
    glm::vec2 behindPos = playerPos + behindDir * flankDistance;
    glm::vec2 leftPos = playerPos + leftDir * flankDistance;
    glm::vec2 rightPos = playerPos + rightDir * flankDistance;
    
    // Check which positions are valid (not in walls)
    bool behindValid = true;
    bool leftValid = true;
    bool rightValid = true;
    
    // Use DDACalculator to check if these positions are reachable (not in walls)
    DDACalculator* ddaCalc = DDACalculator::GetInstance();
    if (ddaCalc) {
        glm::vec2 behindEndpoint = ddaCalc->GetEndpoint(playerPos, behindPos);
        glm::vec2 leftEndpoint = ddaCalc->GetEndpoint(playerPos, leftPos);
        glm::vec2 rightEndpoint = ddaCalc->GetEndpoint(playerPos, rightPos);
        
        // If the endpoint doesn't match our target position, there's a wall in the way
        behindValid = (glm::length(behindEndpoint - behindPos) < 0.1f);
        leftValid = (glm::length(leftEndpoint - leftPos) < 0.1f);
        rightValid = (glm::length(rightEndpoint - rightPos) < 0.1f);
    } else {
        // Fallback: use GetTile and check for wall tiles
        glm::ivec2 behindTile = labManager->GetTilePosition(behindPos);
        glm::ivec2 leftTile = labManager->GetTilePosition(leftPos);
        glm::ivec2 rightTile = labManager->GetTilePosition(rightPos);
        
        int behindTileID = labManager->GetTile(behindTile.x, behindTile.y);
        int leftTileID = labManager->GetTile(leftTile.x, leftTile.y);
        int rightTileID = labManager->GetTile(rightTile.x, rightTile.y);
        
        // Check if tiles are walls using DDACalculator's static method
        behindValid = !(behindTileID >= Tile::WallBottomLeft && behindTileID <= Tile::WallTop);
        leftValid = !(leftTileID >= Tile::WallBottomLeft && leftTileID <= Tile::WallTop);
        rightValid = !(rightTileID >= Tile::WallBottomLeft && rightTileID <= Tile::WallTop);
    }
    
    // Find the closest valid position
    std::vector<std::pair<glm::vec2, float>> validPositions;
    
    if (behindValid) {
        float distToBehind = glm::length(behindPos - selfPos);
        validPositions.push_back({behindPos, distToBehind});
    }
    
    if (leftValid) {
        float distToLeft = glm::length(leftPos - selfPos);
        validPositions.push_back({leftPos, distToLeft});
    }
    
    if (rightValid) {
        float distToRight = glm::length(rightPos - selfPos);
        validPositions.push_back({rightPos, distToRight});
    }
    
    // If no valid positions, return current position
    if (validPositions.empty()) {
        return selfPos;
    }
    
    // Find the closest valid position
    std::sort(validPositions.begin(), validPositions.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return validPositions[0].first;
}

// Helper function to check if we're in a good position for a flanking attack
bool MinitaurController::IsInFlankingPosition() {
    if (!m_pTarget || !m_pTransform) return false;
    
    glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    
    // Check if we're in melee range
    float distance = glm::length(playerPos - selfPos);
    if (distance > m_meleeRange) return false;
    
    // Try to determine if we're behind or to the side of the player
    auto* playerController = m_pTarget->GetComponent<PlayerController>();
    if (playerController) {
        glm::vec2 playerFacingDir = playerController->GetLastFacingDirectionVector();
        glm::vec2 toSelf = glm::normalize(selfPos - playerPos);
        
        // Calculate dot product to determine if we're behind the player
        // A negative dot product means we're more than 90 degrees from where they're facing
        float dotProduct = glm::dot(playerFacingDir, toSelf);
        
        // If dot product is negative, we're behind or to the side
        return dotProduct < 0.0f;
    }
    
    // Default to yes if we can't determine player facing
    return true;
}

// Helper method to set up the flanking behavior sequence
void MinitaurController::SetupFlankingBehavior(std::unique_ptr<Selector>& root) {
    // === FLANKING STRATEGY SEQUENCE ===
    auto flankingSequence = std::make_unique<Sequence>();
    
    // Condition: Check if we're in flanking strategy
    flankingSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { return g_blackboard.GetStrategy() == Strategy::FLANKING; }
    ));
    
    // Condition: Check if player is occupied with another enemy
    flankingSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() {
            int attackingID = g_blackboard.GetAttackingEnemyID();
            int myID = GetGameObject()->GetID();
            
            // If nobody is attacking or we're the attacker, no need to flank
            if (attackingID == -1 || attackingID == myID) {
                return false;
            }
            
            // Otherwise, someone else has the player's attention, good time to flank
            return true;
        }
    ));
    
    // Action: Move to flanking position
    flankingSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            if (!m_pTarget || !m_pTransform || !m_pVelocity) {
                return BehaviorNode::Status::FAILURE;
            }
            
            // Calculate the best flanking position
            glm::vec2 flankPos = GetOptimalAttackPosition();
            g_blackboard.SetVector2("flankPosition", flankPos);
            
            // Move towards the flanking position
            glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
            glm::vec2 moveDir = glm::normalize(flankPos - selfPos);
            
            // Check if another enemy is in the way
            bool isBlocked = false;
            for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
                if (controller.GetGameObject() != GetGameObject()) {
                    glm::vec2 otherPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    
                    // Check if other enemy is between us and our target position
                    glm::vec2 toOther = otherPos - selfPos;
                    glm::vec2 toFlank = flankPos - selfPos;
                    float distToFlank = glm::length(toFlank);
                    float distToOther = glm::length(toOther);
                    
                    // If other enemy is close and in our path
                    if (distToOther < 50.0f && distToOther < distToFlank) {
                        float dot = glm::dot(glm::normalize(toOther), glm::normalize(toFlank));
                        if (dot > 0.7f) {
                            isBlocked = true;
                            // Add avoidance vector to move direction
                            glm::vec2 avoidDir = glm::normalize(selfPos - otherPos);
                            moveDir = glm::normalize(moveDir * 0.3f + avoidDir * 0.7f);
                            break;
                        }
                    }
                }
            }
            
            m_pVelocity->SetVelocity(moveDir * m_chaseSpeed * (isBlocked ? 0.7f : 1.0f));
            
            // Check if we've reached the flanking position
            float distanceToFlankPos = glm::length(flankPos - selfPos);
            
            if (distanceToFlankPos <= 10.0f || IsInFlankingPosition()) {
                return BehaviorNode::Status::SUCCESS;
            }
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Action: Attack from flank
    flankingSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            // Check if we can attack
            if (!m_pTarget || !m_pTransform) {
                return BehaviorNode::Status::FAILURE;
            }
            
            // Check if player is in range for a flank attack
            glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
            float distance = glm::length(targetPos - selfPos);
            
            if (distance <= m_meleeRange) {
                // We're in position, start flanking attack
                ChangeState(EnemyState::ATTACKING);
                
                // Claim attack turn
                g_blackboard.SetAttackingEnemyID(GetGameObject()->GetID());
                
                // Set emote (!) to indicate we're attacking
                SetEmote(EnemyEmote::EXCLAMATION);
                
                // Flanking attacks are fast with a slight damage bonus
                m_meleeWindupTimer = m_meleeWindupTime * 0.8f;
                g_blackboard.SetFloat("attackDamageMultiplier", 1.5f);
                
                return BehaviorNode::Status::SUCCESS;
            }
            
            // If not in range, keep trying to move to attack position
            glm::vec2 moveDir = glm::normalize(targetPos - selfPos);
            m_pVelocity->SetVelocity(moveDir * m_chaseSpeed);
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add the flanking sequence to the root selector
    root->AddBehaviorNode(std::move(flankingSequence));
}

// Setup the complete behavior tree for combat
void MinitaurController::SetupCombatBehaviorTree() {
    // Create the root selector
    auto root = std::make_unique<Selector>();
    
    // === DEFENSIVE STRATEGY SEQUENCE (DODGING) ===
    auto defensiveSequence = std::make_unique<Sequence>();
    
    // Condition: Check if we're in defensive strategy
    defensiveSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { return g_blackboard.GetStrategy() == Strategy::DEFENSIVE; }
    ));
    
    // Condition: Check if player is attacking
    defensiveSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { return ShouldDodgePlayerAttack(); }
    ));
    
    // Action: Perform dodge
    defensiveSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            // Calculate dodge direction (perpendicular to player direction)
            if (!m_pTarget || !m_pVelocity || !m_pTransform) {
                return BehaviorNode::Status::FAILURE;
            }
            
            // Get player direction
            glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
            glm::vec2 toPlayer = glm::normalize(playerPos - selfPos);
            
            // Calculate perpendicular directions (left or right)
            glm::vec2 perpendicularLeft(-toPlayer.y, toPlayer.x);
            glm::vec2 perpendicularRight(toPlayer.y, -toPlayer.x);
            
            // Choose which direction to dodge (away from walls)
            glm::vec2 dodgeDir;
            
            // Use DDACalculator to check for walls in both directions
            DDACalculator* ddaCalc = DDACalculator::GetInstance();
            if (ddaCalc) {
                // Sample points ~2 tiles in both perpendicular directions
                glm::vec2 leftPos = selfPos + perpendicularLeft * 96.0f;
                glm::vec2 rightPos = selfPos + perpendicularRight * 96.0f;
                
                // Use GetEndpoint to check if there's a wall in the way
                glm::vec2 leftEndpoint = ddaCalc->GetEndpoint(selfPos, leftPos);
                glm::vec2 rightEndpoint = ddaCalc->GetEndpoint(selfPos, rightPos);
                
                // If the endpoint doesn't match our target position, there's a wall in the way
                bool leftIsWall = (glm::length(leftEndpoint - leftPos) > 0.1f);
                bool rightIsWall = (glm::length(rightEndpoint - rightPos) > 0.1f);
                
                // Choose the direction that isn't a wall, or random if both are clear
                if (leftIsWall && !rightIsWall) {
                    dodgeDir = perpendicularRight;
                } else if (!leftIsWall && rightIsWall) {
                    dodgeDir = perpendicularLeft;
                } else {
                    // If both are clear or both are walls, choose randomly
                    dodgeDir = (m_RNG.NextInt(0, 1) == 0) ? perpendicularLeft : perpendicularRight;
                }
            } else if (m_pPathfindingManager) {
                // Fallback to using tile checks directly
                auto* labManager = m_pPathfindingManager->GetLabyrinthManager();
                
                // Sample points ~2 tiles in both perpendicular directions
                glm::vec2 leftPos = selfPos + perpendicularLeft * 96.0f;
                glm::vec2 rightPos = selfPos + perpendicularRight * 96.0f;
                
                // Convert to tile positions and check for walls
                glm::ivec2 leftTile = labManager->GetTilePosition(leftPos);
                glm::ivec2 rightTile = labManager->GetTilePosition(rightPos);
                
                int leftTileID = labManager->GetTile(leftTile.x, leftTile.y);
                int rightTileID = labManager->GetTile(rightTile.x, rightTile.y);
                
                // Check if tiles are walls using the Tile enum range
                bool leftIsWall = (leftTileID >= Tile::WallBottomLeft && leftTileID <= Tile::WallTop);
                bool rightIsWall = (rightTileID >= Tile::WallBottomLeft && rightTileID <= Tile::WallTop);
                
                // Choose the direction that isn't a wall, or random if both are clear
                if (leftIsWall && !rightIsWall) {
                    dodgeDir = perpendicularRight;
                } else if (!leftIsWall && rightIsWall) {
                    dodgeDir = perpendicularLeft;
                } else {
                    // If both are clear or both are walls, choose randomly
                    dodgeDir = (m_RNG.NextInt(0, 1) == 0) ? perpendicularLeft : perpendicularRight;
                }
            } else {
                // Fallback to random direction if no pathfinding manager or DDACalculator
                dodgeDir = (m_RNG.NextInt(0, 1) == 0) ? perpendicularLeft : perpendicularRight;
            }
            
            // Apply dodge movement (reduced strength from 500.0f to 300.0f)
            m_pVelocity->ApplyKnockback(dodgeDir, 300.0f);
            

            
            // Set dodge timer (to track when we should counter-attack) - shorter time
            g_blackboard.SetFloat("dodgeTimer", 0.0f); 
            g_blackboard.SetBool("isDodging", true);
            
            // Set emote (!!)
            SetEmote(EnemyEmote::EXCLAMATION);
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Action: Wait for dodge to complete, then counter-attack
    defensiveSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            // Update dodge timer
            float dodgeTimer = g_blackboard.GetFloat("dodgeTimer") + delta;
            g_blackboard.SetFloat("dodgeTimer", dodgeTimer);
            
            // Dodge lasts for 0.3 seconds (reduced from 0.5f)
            if (dodgeTimer < 0.3f) {
                return BehaviorNode::Status::RUNNING;
            }
            
            // End of dodge, start counter-attack
            g_blackboard.SetBool("isDodging", false);
            
            // Only counter-attack if we have a clear turn and the player is in range
            int attackingID = g_blackboard.GetAttackingEnemyID();
            int myID = GetGameObject()->GetID();
            bool myTurn = (attackingID == -1 || attackingID == myID);
            
            if (myTurn && m_pTarget) {
                glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
                float distance = glm::length(targetPos - selfPos);
                
                if (distance <= m_meleeRange * 1.5f) { // Slightly increased range for counter-attack
                    // Start attacking state
                    ChangeState(EnemyState::ATTACKING);
                    
                    // Claim attack turn
                    g_blackboard.SetAttackingEnemyID(myID);
                    
                    // Quick counter-attack (faster windup, normal damage)
                    m_meleeWindupTimer = m_meleeWindupTime * 0.7f;
                    g_blackboard.SetFloat("attackDamageMultiplier", 1.0f);
                }
            }
            
            return BehaviorNode::Status::SUCCESS;
        }
    ));
    
    // Add defensive sequence to root
    root->AddBehaviorNode(std::move(defensiveSequence));
    
    // Add flanking sequence
    SetupFlankingBehavior(root);
    
    // === AGGRESSIVE STRATEGY SEQUENCE ===
    auto aggressiveSequence = std::make_unique<Sequence>();
    
    // Condition: Check if we're in aggressive strategy
    aggressiveSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { return g_blackboard.GetStrategy() == Strategy::AGGRESSIVE; }
    ));
    
    // Condition: Check if player is in melee range
    aggressiveSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() {
            if (!m_pTarget) return false;
            
            glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
            float distance = glm::length(targetPos - selfPos);
            
            return distance <= m_meleeRange;
        }
    ));
    
    // Condition: Check if it's our turn to attack
    aggressiveSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() {
            int attackingID = g_blackboard.GetAttackingEnemyID();
            int myID = GetGameObject()->GetID();
            
            return attackingID == -1 || attackingID == myID;
        }
    ));
    
    // Action: Perform heavy attack
    aggressiveSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            // If not already attacking, enter attack state
            if (m_state != EnemyState::ATTACKING) {
                ChangeState(EnemyState::ATTACKING);
                
                // Set as attacking enemy
                g_blackboard.SetAttackingEnemyID(GetGameObject()->GetID());
                
                // Set emote (!) to indicate an aggressive attack
                SetEmote(EnemyEmote::EXCLAMATION);
                
                // Use a longer windup for heavy attack
                m_meleeWindupTimer = m_meleeWindupTime * 1.5f;
                
                // Store damage multiplier in blackboard
                g_blackboard.SetFloat("attackDamageMultiplier", 2.0f);
                
                return BehaviorNode::Status::RUNNING;
            }
            
            // Check if attack is complete
            if (m_state != EnemyState::ATTACKING) {
                g_blackboard.SetAttackingEnemyID(-1); // Release attack turn
                return BehaviorNode::Status::SUCCESS;
            }
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add aggressive sequence to root
    root->AddBehaviorNode(std::move(aggressiveSequence));
    
    // === REGULAR ATTACK FALLBACK (when no specific strategy conditions are met) ===
    auto regularAttackSequence = std::make_unique<Sequence>();
    
    // Condition: Check if player is in melee range
    regularAttackSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() {
            if (!m_pTarget) return false;
            
            glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
            float distance = glm::length(targetPos - selfPos);
            
            return distance <= m_meleeRange;
        }
    ));
    
    // Condition: Check if it's our turn to attack
    regularAttackSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() {
            int attackingID = g_blackboard.GetAttackingEnemyID();
            int myID = GetGameObject()->GetID();
            
            return attackingID == -1 || attackingID == myID;
        }
    ));
    
    // Action: Perform regular attack
    regularAttackSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            // If not already attacking, enter attack state
            if (m_state != EnemyState::ATTACKING) {
                ChangeState(EnemyState::ATTACKING);
                
                // Set as attacking enemy
                g_blackboard.SetAttackingEnemyID(GetGameObject()->GetID());
                
                // Normal windup time and damage for regular attack
                m_meleeWindupTimer = m_meleeWindupTime;
                g_blackboard.SetFloat("attackDamageMultiplier", 1.0f);
                
                return BehaviorNode::Status::RUNNING;
            }
            
            // Check if attack is complete
            if (m_state != EnemyState::ATTACKING) {
                g_blackboard.SetAttackingEnemyID(-1); // Release attack turn
                return BehaviorNode::Status::SUCCESS;
            }
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add regular attack sequence to root
    root->AddBehaviorNode(std::move(regularAttackSequence));
    
    // Create behavior tree with root
    m_combatBehaviorTree = std::make_unique<BehaviorTree>(std::move(root));
}

void MinitaurController::UpdateBlackboard() {
    if (!m_pTarget || !m_pTransform) return;
    
    // Calculate these values only once
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    float distanceToPlayer = glm::length(targetPos - selfPos);
    
    g_blackboard.SetFloat("playerDistance", distanceToPlayer);
    
    // Update health data - Health doesn't change that frequently
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
    
    // Only count nearby enemies every 0.5 seconds, using a static counter
    static float s_enemyCountTimer = 0.0f;
    static int s_cachedNearbyEnemies = 0;
    
    s_enemyCountTimer += 0.1f; // Approximate the delta time
    if (s_enemyCountTimer > 0.5f) {
        s_cachedNearbyEnemies = 0;
        for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
            if (controller.GetGameObject() != GetGameObject() && controller.GetTarget() == m_pTarget) {
                glm::vec2 enemyPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                float distanceToEnemy = glm::length(enemyPos - selfPos);
                
                if (distanceToEnemy < m_detectionRange * 1.5f) {
                    s_cachedNearbyEnemies++;
                }
            }
        }
        g_blackboard.SetInt("nearbyEnemies", s_cachedNearbyEnemies);
        s_enemyCountTimer = 0.0f;
    }
}

void MinitaurController::EvaluateStrategy() {
    if (!m_pTarget || !m_pHealth) return;
    
    // Cache values we'll use multiple times
    int myID = GetGameObject()->GetID();
    float selfHealth = m_pHealth->GetHealth();
    float healthPercentage = (selfHealth / m_pHealth->GetMaxHealth()) * 100.0f;
    
    // Check for critical conditions first
    if (healthPercentage < 30.0f) {
        g_blackboard.SetStrategy(Strategy::DEFENSIVE);
        return;
    }
    
    float playerHealth = 100.0f;
    auto* playerHealthComp = m_pTarget->GetComponent<HealthComponent>();
    if (playerHealthComp) {
        playerHealth = playerHealthComp->GetHealth();
        if (playerHealth < 20.0f) {
            g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
            return;
        }
    }
    
    // Use cached enemy count from blackboard instead of recounting
    int nearbyEnemies = g_blackboard.GetInt("nearbyEnemies");
    
    // Fast path decision based on group size
    if (nearbyEnemies >= 1) {
        int attackingID = g_blackboard.GetAttackingEnemyID();
        
        // If someone else is attacking, always flank unless we're designated defensive
        if (attackingID != -1 && attackingID != myID) {
            // Simple modulo rule instead of complex angle calculations
            g_blackboard.SetStrategy((myID % 3 == 2) ? Strategy::DEFENSIVE : Strategy::FLANKING);
            return;
        }
        
        // Use modulo to distribute roles consistently
        int roleSelector = myID % 3;
        switch (roleSelector) {
            case 0: 
                g_blackboard.SetStrategy(Strategy::AGGRESSIVE); 
                break;
            case 1: 
                g_blackboard.SetStrategy(Strategy::FLANKING); 
                break;
            case 2:
                // Quick distance calculation for defensive vs flanking decision
                float distanceToPlayer = g_blackboard.GetFloat("playerDistance");
                g_blackboard.SetStrategy(distanceToPlayer < m_meleeRange * 1.5f ? 
                    Strategy::DEFENSIVE : Strategy::FLANKING);
                break;
        }
    }
    else {
        // When solo, use more dynamic strategy switching based on health and player position
        auto* playerController = m_pTarget->GetComponent<PlayerController>();
        bool playerIsAttacking = false;
        
        if (playerController) {
            playerIsAttacking = playerController->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING;
        }
        
        // Higher chance of defensive when health is lower or player is attacking
        float defensiveChance = 0.5f;
        
        // Adjust chance based on health (more defensive at lower health)
        defensiveChance += (100.0f - healthPercentage) / 100.0f;
        
        // Much higher chance of defensive when player is attacking
        if (playerIsAttacking) {
            defensiveChance += 0.75f;
        }
        
        // Check distance to player - be more defensive when close
        float distanceToPlayer = g_blackboard.GetFloat("playerDistance");
        if (distanceToPlayer < m_meleeRange * 2.0f) {
            defensiveChance += 0.25f;
        }
        
        // Add some randomization to make behavior less predictable
        float randomValue = m_RNG.NextFloat(0.0f, 1.0f);
        
        // Apply defensive strategy if the random value is below our calculated chance
        g_blackboard.SetStrategy(randomValue < defensiveChance ? Strategy::DEFENSIVE : Strategy::AGGRESSIVE);
    }
}

glm::vec2 MinitaurController::GetOptimalAttackPosition() 
{
    if (!m_pTarget || !m_pTransform) return m_pTransform->GetGlobalPosition();
    
    glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    
    // Create 8 potential positions around the player
    std::vector<glm::vec2> potentialPositions;
    float attackDistance = m_meleeRange * 0.9f; // Slightly inside melee range
    
    // Generate positions in 8 directions
    for (int i = 0; i < 8; i++) {
        float angle = i * glm::pi<float>() / 4.0f;
        glm::vec2 offset(cos(angle) * attackDistance, sin(angle) * attackDistance);
        potentialPositions.push_back(playerPos + offset);
    }
    
    // Filter out positions that are in walls
    std::vector<glm::vec2> validPositions;
    DDACalculator* ddaCalc = DDACalculator::GetInstance();
    
    for (const auto& pos : potentialPositions) {
        glm::vec2 endpoint = ddaCalc->GetEndpoint(playerPos, pos);
        if (glm::length(endpoint - pos) < 0.1f) {
            validPositions.push_back(pos);
        }
    }
    
    if (validPositions.empty()) return playerPos; // Fallback
    
    // Score each position based on:
    // 1. Distance from current position (closer is better)
    // 2. Whether it's already occupied by another enemy (avoid)
    // 3. Whether it's behind the player (preferred)
    
    glm::vec2 bestPosition = validPositions[0];
    float bestScore = -1000.0f;
    
    for (const auto& pos : validPositions) {
        float score = 0.0f;
        
        // Distance factor - prefer closer positions
        float distance = glm::length(pos - m_pTransform->GetGlobalPosition());
        score -= distance * 0.1f;
        
        // Occupation factor - avoid positions with other enemies
        if (IsPositionOccupiedByEnemy(pos)) {
            score -= 50.0f;
        }
        
        // Behind player factor - prefer positions behind player
        auto* playerController = m_pTarget->GetComponent<PlayerController>();
        if (playerController) {
            glm::vec2 playerFacing = playerController->GetLastFacingDirectionVector();
            glm::vec2 toPosition = glm::normalize(pos - playerPos);
            float dotProduct = glm::dot(playerFacing, toPosition);
            
            // Negative dot product means we're behind
            if (dotProduct < 0) {
                score += 20.0f;
            }
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestPosition = pos;
        }
    }
    
    return bestPosition;
}

bool MinitaurController::IsPositionOccupiedByEnemy(const glm::vec2& position) 
{
    for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
        if (controller.GetGameObject() != GetGameObject()) {
            glm::vec2 enemyPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            float distance = glm::length(enemyPos - position);
            
            if (distance < 40.0f) {
                return true;
            }
        }
    }
    return false;
}

