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
    m_pAnimComponent->SetLightingEnabled(false);
}

void MinitaurController::RenderDebugPath()
{

}

void MinitaurController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform)
        return;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    
    // STEP 1: Try NavMesh pathfinding first
    if (m_pNavMeshComponent && m_useNavMesh)
    {
        // Update path periodically
        if (m_navMeshPath.empty() || m_navMeshPathUpdateTimer <= 0.0f)
        {
            // Try full NavMesh pathfinding first
            m_navMeshPath = m_pNavMeshComponent->FindPath(currentPosition, targetPosition);
            
            // Validate the path
            if (m_navMeshPath.size() >= 2 && !m_pNavMeshComponent->IsPathValid(m_navMeshPath))
            {
                m_navMeshPath = m_pNavMeshComponent->CreateGridBasedPath(currentPosition, targetPosition);
                
                if (m_navMeshPath.size() < 2)
                {
                    m_useNavMesh = false;
                }
            }
            else if (m_navMeshPath.size() < 2)
            {
                m_useNavMesh = false;
            }
            
            m_navMeshPathUpdateTimer = 0.5f;
        }
        else
        {
            m_navMeshPathUpdateTimer -= delta;
        }
        
        // Follow NavMesh path if valid
        if (m_navMeshPath.size() >= 2)
        {
            glm::vec2 nextPoint = m_navMeshPath[1];
            glm::vec2 direction = nextPoint - currentPosition;
            float distance = glm::length(direction);
            
            // More generous tolerance for waypoint arrival
            if (distance <= 10.0f)
            {
                m_navMeshPath.erase(m_navMeshPath.begin());
            }
            else
            {
                direction = glm::normalize(direction);
                m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                return; // Successfully using NavMesh
            }
        }
    }
    
    // STEP 2: Fall back to grid-based pathfinding
    if (m_pPathfindingManager)
    {
        auto& pathData = m_pPathfindingManager->GetPathData(GetGameObject());
        
        // If we need a new grid path
        if (pathData.path.empty() || pathData.targetTile != glm::ivec2(targetPosition) / 
                                     (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE))
        {
            glm::ivec2 currentTile = glm::ivec2(currentPosition) / 
                                    (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);
            glm::ivec2 targetTile = glm::ivec2(targetPosition) / 
                                   (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);
            
            
            // Explicitly request a new path
            std::vector<glm::ivec2> newPath = m_pPathfindingManager->FindPath(currentTile, targetTile);
            
            if (!newPath.empty())
            {
                pathData.path = newPath;
                pathData.targetTile = targetTile;
            }
        }
        
        if (!pathData.path.empty())
        {
            glm::ivec2 nextTile = pathData.path.front();
            glm::vec2 nextTileWorldPos = m_pPathfindingManager->GetLabyrinthManager()->GetWorldPosition(nextTile) + 
                                       glm::vec2(48.0f, 48.0f);
            glm::vec2 direction = nextTileWorldPos - currentPosition;
            float distance = glm::length(direction);
            
            if (distance > 0.5f)
            {
                direction = glm::normalize(direction);
                m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                return; // Successfully using grid pathfinding
            }
            else
            {
                // Advance to the next waypoint
                pathData.path.erase(pathData.path.begin());
                return;
            }
        }
    }
    
    // STEP 3: Fall back to direct distance checking
    FallbackToDistanceChecking();
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
// ===== DODGE DETECTION =====
bool MinitaurController::ShouldDodgePlayerAttack() {
    if (!m_pTarget) return false;
    
    auto* playerController = m_pTarget->GetComponent<PlayerController>();
    if (!playerController || playerController->GetPlayerAction() != PlayerController::PlayerAction::ATTACKING) 
        return false;
    
    // Quick distance and facing check
    glm::vec2 toPlayer = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - 
                         m_pTransform->GetGlobalPosition();
    return glm::length(toPlayer) <= 80.0f && 
           glm::dot(playerController->GetLastFacingDirectionVector(), glm::normalize(-toPlayer)) > 0.7f;
}

// Keep HandleDodgeState as-is since it's just calling DodgeFromPlayer
void MinitaurController::HandleDodgeState(float delta)
{
    DodgeFromPlayer(delta);
}

void MinitaurController::DodgeFromPlayer(float delta) {
    // Initialize dodge if needed
    if (!m_isDodging) {
        m_isDodging = true;
        m_dodgeTimer.Restart();
        
        // Calculate dodge direction
        glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
        glm::vec2 toPlayer = glm::normalize(playerPos - selfPos);
        
        // Check if ranged weapon
        bool isRanged = false;
        auto* playerController = m_pTarget->GetComponent<PlayerController>();
        if (playerController) {
            WeaponItem* pWeapon = playerController->GetHeldWeapon();
            isRanged = pWeapon && pWeapon->GetWeaponType() == WeaponType::BOW;
        }
        
        if (isRanged) {
            // Sideways dodge (perpendicular)
            float angle = m_RNG.FlipCoin() ? 90.0f : -90.0f;
            glm::mat4 rotation = glm::rotate(glm::radians(angle), glm::vec3(0, 0, 1));
            m_dodgeDir = glm::vec2(rotation * glm::vec4(toPlayer, 0, 1));
        } else {
            // Backward dodge
            m_dodgeDir = -toPlayer;
        }
        
        m_pVelocity->ApplyKnockback(m_dodgeDir, 400.0f);
        SetEmote(EnemyEmote::EXCLAMATION);
    }
    
    // Continue or end dodge
    if (m_dodgeTimer.Elapsed() < 0.4f) {
        m_pVelocity->SetVelocity(m_dodgeDir * m_chaseSpeed * 1.1f);
    } else {
        m_isDodging = false;
        ChangeState(EnemyState::CHASING);
    }
}

// ===== STRATEGY EVALUATION =====
void MinitaurController::EvaluateStrategy() {
    if (!m_pTarget || !m_pHealth) return;
    
    float healthPercent = (m_pHealth->GetHealth() / m_pHealth->GetMaxHealth()) * 100.0f;
    int nearbyEnemies = g_blackboard.GetInt("nearbyEnemies");
    int myID = GetGameObject()->GetID();
    
    // Critical health check
    if (healthPercent < 30.0f) {
        g_blackboard.SetStrategy(Strategy::DEFENSIVE);
        return;
    }
    
    // Wounded player check
    auto* playerHealth = m_pTarget->GetComponent<HealthComponent>();
    if (playerHealth && playerHealth->GetHealth() < 20.0f) {
        g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
        return;
    }
    
    // Group tactics vs solo tactics
    if (nearbyEnemies >= 1) {
        int attackingID = g_blackboard.GetAttackingEnemyID();
        
        if (attackingID != -1 && attackingID != myID) {
            // Someone else is attacking - flank or defend based on ID
            g_blackboard.SetStrategy((myID % 2 == 0) ? Strategy::FLANKING : Strategy::DEFENSIVE);
        } else {
            // Simple role distribution by ID
            g_blackboard.SetStrategy(Strategy(myID % 3));  // Maps to 0=AGGRESSIVE, 1=FLANKING, 2=DEFENSIVE
        }
    } else {
        // Solo - health-based with randomization
        g_blackboard.SetStrategy(m_RNG.NextFloat(0.0f, 100.0f) < (100.0f - healthPercent) ?
                              Strategy::DEFENSIVE : Strategy::AGGRESSIVE);
    }
}

// ===== SIMPLIFIED POSITION FINDER =====
glm::vec2 MinitaurController::GetOptimalAttackPosition() {
    if (!m_pTarget || !m_pTransform) return m_pTransform->GetGlobalPosition();
    
    glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    glm::vec2 toPlayer = glm::normalize(playerPos - selfPos);
    float attackDist = m_meleeRange * 0.9f;
    
    // Default is direct approach
    glm::vec2 targetPos = playerPos - toPlayer * attackDist;
    
    // Adjust based on strategy
    Strategy strategy = g_blackboard.GetStrategy();
    if (strategy == Strategy::FLANKING) {
        auto* playerController = m_pTarget->GetComponent<PlayerController>();
        if (playerController) {
            // Get side position (perpendicular to player facing)
            glm::vec2 playerFacing = playerController->GetLastFacingDirectionVector();
            glm::vec2 sideDir = m_RNG.FlipCoin() ? 
                glm::vec2(-playerFacing.y, playerFacing.x) : 
                glm::vec2(playerFacing.y, -playerFacing.x);
            
            targetPos = playerPos + sideDir * attackDist;
        }
    } else if (strategy == Strategy::DEFENSIVE) {
        // Stay a bit further back
        targetPos = playerPos - toPlayer * (attackDist * 1.2f);
    }
    
    return targetPos;
}

// Simple helper to check for other Minitaurs
bool MinitaurController::IsPositionOccupiedByEnemy(const glm::vec2& position) {
    int myID = GetGameObject()->GetID();
    
    for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
        if (controller.GetGameObject()->GetID() != myID) {
            glm::vec2 enemyPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            if (glm::length(enemyPos - position) < 40.0f) return true;
        }
    }
    return false;
}

// ===== BEHAVIOR TREE SETUP =====
void MinitaurController::SetupCombatBehaviorTree() {
    auto root = std::make_unique<Selector>();
    
    // === DODGE BEHAVIOR ===
    auto dodgeSequence = std::make_unique<Sequence>();
    dodgeSequence->AddBehaviorNode(std::make_unique<ConditionNode>([this]() { 
        return ShouldDodgePlayerAttack(); 
    }));
    
    dodgeSequence->AddBehaviorNode(std::make_unique<ActionNode>([this](float delta) {
        if (!m_pTarget || !m_pVelocity) return BehaviorNode::Status::FAILURE;
        
        // Calculate dodge direction
        glm::vec2 toPlayer = glm::normalize(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - 
                                           m_pTransform->GetGlobalPosition());
        glm::vec2 dodgeDir = m_RNG.FlipCoin() ? 
            glm::vec2(-toPlayer.y, toPlayer.x) : glm::vec2(toPlayer.y, -toPlayer.x);
        
        m_pVelocity->ApplyKnockback(dodgeDir, 300.0f);
        SetEmote(EnemyEmote::EXCLAMATION);
        
        return BehaviorNode::Status::SUCCESS;
    }));
    
    root->AddBehaviorNode(std::move(dodgeSequence));
    
    // === ATTACK BEHAVIOR ===
    auto attackSequence = std::make_unique<Sequence>();
    
    // Condition: in range and eligible to attack
    attackSequence->AddBehaviorNode(std::make_unique<ConditionNode>([this]() {
        if (!m_pTarget) return false;
        
        float distance = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - 
                                    m_pTransform->GetGlobalPosition());
        int attackingID = g_blackboard.GetAttackingEnemyID();
        
        return distance <= m_meleeRange && (attackingID == -1 || attackingID == GetGameObject()->GetID());
    }));
    
    // Action: perform attack
    attackSequence->AddBehaviorNode(std::make_unique<ActionNode>([this](float delta) {
        if (m_state != EnemyState::ATTACKING) {
            ChangeState(EnemyState::ATTACKING);
            g_blackboard.SetAttackingEnemyID(GetGameObject()->GetID());
            
            // Set attack properties based on strategy
            Strategy strategy = g_blackboard.GetStrategy();
            float windupMult = 1.0f;
            float damageMult = 1.0f;
            
            switch (strategy) {
                case Strategy::AGGRESSIVE:
                    windupMult = 1.2f; 
                    damageMult = 1.5f;
                    break;
                case Strategy::FLANKING:
                    windupMult = 0.8f;
                    damageMult = 1.2f;
                    break;
                default: break; // Default values already set
            }
            
            m_meleeWindupTimer = m_meleeWindupTime * windupMult;
            g_blackboard.SetFloat("attackDamageMultiplier", damageMult);
            SetEmote(EnemyEmote::EXCLAMATION);
        }
        
        return (m_state == EnemyState::ATTACKING) ? 
               BehaviorNode::Status::RUNNING : BehaviorNode::Status::SUCCESS;
    }));
    
    root->AddBehaviorNode(std::move(attackSequence));
    
    // === POSITIONING BEHAVIOR ===
    auto positionSequence = std::make_unique<Sequence>();
    
    // Condition: not in attack range
    positionSequence->AddBehaviorNode(std::make_unique<ConditionNode>([this]() {
        if (!m_pTarget) return false;
        float distance = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - 
                                    m_pTransform->GetGlobalPosition());
        return distance > m_meleeRange;
    }));
    
    // Action: move to optimal position
    positionSequence->AddBehaviorNode(std::make_unique<ActionNode>([this](float delta) {
        if (!m_pTarget || !m_pVelocity) return BehaviorNode::Status::FAILURE;
        
        glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
        glm::vec2 moveDir;
        
        // Position based on strategy
        Strategy strategy = g_blackboard.GetStrategy();
        if (strategy == Strategy::FLANKING && g_blackboard.GetInt("nearbyEnemies") > 0) {
            auto* playerController = m_pTarget->GetComponent<PlayerController>();
            if (playerController) {
                glm::vec2 playerFacing = playerController->GetLastFacingDirectionVector();
                float angle = m_RNG.NextFloat(120.0f, 240.0f) * (glm::pi<float>() / 180.0f);
                glm::vec2 flankDir = glm::vec2(cos(angle) * playerFacing.x - sin(angle) * playerFacing.y,
                                             sin(angle) * playerFacing.x + cos(angle) * playerFacing.y);
                moveDir = glm::normalize(targetPos + flankDir * m_meleeRange * 0.9f - selfPos);
            } else {
                moveDir = glm::normalize(targetPos - selfPos);
            }
        } else {
            moveDir = glm::normalize(targetPos - selfPos);
        }
        
        // Apply basic avoidance from other Minitaurs
        int myID = GetGameObject()->GetID();
        for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
            if (controller.GetGameObject()->GetID() != myID) {
                glm::vec2 otherPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                float dist = glm::length(otherPos - selfPos);
                if (dist < 70.0f) {
                    moveDir = glm::normalize(moveDir * 0.7f + glm::normalize(selfPos - otherPos) * 0.3f);
                    break;
                }
            }
        }
        
        m_pVelocity->SetVelocity(moveDir * m_chaseSpeed);
        return BehaviorNode::Status::RUNNING;
    }));
    
    root->AddBehaviorNode(std::move(positionSequence));
    
    m_combatBehaviorTree = std::make_unique<BehaviorTree>(std::move(root));
}

// ===== BLACKBOARD UPDATE =====
void MinitaurController::UpdateBlackboard() {
    if (!m_pTarget || !m_pTransform) return;
    
    // Update basic positional data
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    g_blackboard.SetFloat("playerDistance", glm::length(targetPos - selfPos));
    
    // Update health
    if (m_pHealth) {
        g_blackboard.SetFloat("selfHealthPercentage", 
            (m_pHealth->GetHealth() / m_pHealth->GetMaxHealth()) * 100.0f);
    }
    
    // Count nearby enemies (only occasionally)
    static float enemyCountTimer = 0.0f;
    enemyCountTimer += 0.1f;
    
    if (enemyCountTimer > 0.5f) {
        int count = 0;
        for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<MinitaurController>()) {
            if (controller.GetGameObject() != GetGameObject() && controller.GetTarget() == m_pTarget) {
                float dist = glm::length(controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - selfPos);
                if (dist < m_detectionRange * 1.5f) count++;
            }
        }
        g_blackboard.SetInt("nearbyEnemies", count);
        enemyCountTimer = 0.0f;
    }
}