//-----------------------------------------------------------------------------
// File: HarpyController.cpp - Controls harpy attacks & behaviours
//-----------------------------------------------------------------------------
#include "HarpyController.h"
#include "PlayerController.h"
#include "AttackDamageComponent.h"
#include "HomingComponent.h"
#include "TimedDestroyerComponent.h"
#include "DDACalculator.h"
#include "inventory/ItemDropCreator.h"
#include <LightComponent.h>
#include <math.h>
#include <cassert>

HarpyController::HarpyController() {
    wolf::EventManager::AddListener<InfightingEvent, HarpyController, &HarpyController::HandleInfighting>(*this);
}  

HarpyController::~HarpyController() {
    wolf::EventManager::RemoveListener<InfightingEvent, HarpyController, &HarpyController::HandleInfighting>(*this);
    g_blackboard.UnregisterEnemy(GetGameObject()->GetID());
}

void HarpyController::Init(const EnemyData& data) {
    auto* pGameObject = GetGameObject();
    if (!pGameObject) {
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

    // Get required components
    m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (!m_pVelocity) {
        wolf::Warning("Harpy " + std::to_string(pGameObject->GetID()) + " could not find VelocityComponent!");
    }

    // Set up animations
    SetUpAnimations(data.animationInitFile);

    // Find player target
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        m_pTarget = playerController.GetGameObject();
        break;
    }

    // Init attack state
    m_rangedWindupTimer = m_rangedWindupTime;
    m_attackChain = 0;

    // Init emotes
    m_pEmoteObj = &pGameObject->GetScene().CreateObject2D();
    pGameObject->AddChild(*m_pEmoteObj);
    m_pEmoteObj->GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(-8.0f, 8.0f));
    
    AnimatedSprite2D* emotesSpritesheet = &m_pEmoteObj->AddComponent<AnimatedSprite2D>("data/emotes_anim_init.yaml");
    emotesSpritesheet->SetAnimPaused(true);
    emotesSpritesheet->SetOriginToCenterOfFrame();
    m_fEmoteTimer = EMOTE_TIME;
    
    // Register with blackboard
    g_blackboard.RegisterEnemy(GetGameObject()->GetID());
    
    // Setup combat behavior tree
    SetupCombatBehaviorTree();
}

void HarpyController::Update(float delta) {
    // Skip update if not active or missing components
    if (!m_active || !m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    // Check target health
    if (m_pTarget) {
        auto* targetHealth = m_pTarget->GetComponent<HealthComponent>();
        if (!targetHealth || targetHealth->GetHealth() <= 0) {
            RevertBackToPlayer();
        }
    }
    
    // Update base class
    EnemyController::Update(delta);
    
    // Check for petrification
    StatusComponent* statusComponent = GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)) {
        ChangeState(EnemyState::DEATH);
    }

    // Death check
    if (m_pHealth->GetHealth() <= 0 && m_state != EnemyState::DEATH) {
        ChangeState(EnemyState::DEATH);
        return;
    }

    // Update ranged attack timer
    if(m_rangedTimer > 0.0f) {
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
    if (s_aiUpdateTimers[myID] > 0.2f) {
        EvaluateStrategy();
        UpdateBlackboard();
        s_aiUpdateTimers[myID] = 0.0f;
    }
    
    // Update behavior tree for combat states
    if (m_combatBehaviorTree && (m_state == EnemyState::CHASING || m_state == EnemyState::IDLE || m_state == EnemyState::ATTACKING)) {
        m_combatBehaviorTree->Update(delta, &g_blackboard);
    }

    // State-based updates
    switch (m_state) {
        case EnemyState::IDLE: HandleIdleState(delta); break;
        case EnemyState::CHASING: HandleChasingState(delta); break;
        case EnemyState::ATTACKING: HandleAttackingState(delta); break;
        case EnemyState::PETRIFIED: HandlePetrifiedState(delta); break;
        case EnemyState::PROSPECT: break;
        case EnemyState::STUNNED: HandleStunnedState(delta); break;
        case EnemyState::DEATH: 
            HandleDeathState(delta);
            return;  // Return immediately since object may be deleted
    }

    // Update animations based on direction
    UpdateAnimationBasedOnDirection();

    // Update emote timer
    if(m_fEmoteTimer > 0.0f) {
        m_fEmoteTimer -= delta;
    }
    else if(m_emote != EnemyEmote::NONE) {
        SetEmote(EnemyEmote::NONE);
    }
}

void HarpyController::ChangeState(EnemyState newState) {
    // Exit old state
    switch (m_state) {
        case EnemyState::ATTACKING: ExitAttackState(); break;
        case EnemyState::CHASING: ExitChasingState(); break;
        case EnemyState::IDLE: ExitIdleState(); break;
        case EnemyState::PETRIFIED: ExitPetrifiedState(); break;
        case EnemyState::STUNNED: ExitStunnedState(); break;
        default: break;
    }
    
    // Enter new state
    switch (newState) {
        case EnemyState::ATTACKING: EnterAttackState(); break;
        case EnemyState::CHASING: EnterChasingState(); break;
        case EnemyState::IDLE: EnterIdleState(); break;
        case EnemyState::PETRIFIED: EnterPetrifiedState(); break;
        case EnemyState::STUNNED: EnterStunnedState(); break;
        case EnemyState::DEATH: EnterDeathState(); break;
        default: break;
    }

    m_last_state = m_state;
    m_state = newState;
}

void HarpyController::SetUpAnimations(const std::string& animationInitPath) {
    auto* pGameObject = GetGameObject();
    
    if (pGameObject->HasAll<AnimatedSprite2D>()) {
        pGameObject->DeleteComponent<AnimatedSprite2D>();
    }

    m_pAnimComponent = &GetGameObject()->AddComponent<AnimatedSprite2D>(animationInitPath);
    m_pAnimComponent->SetLayer(11);
    m_pAnimComponent->SetLightingEnabled(false);
}

void HarpyController::MoveTowardsTarget(float delta) {
    if (!m_pTarget || !m_pVelocity || !m_pTransform) return;

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    // Stop if within attack range with line of sight
    if (distanceToPlayer <= m_rangedRange && IsTargetInLOS()) {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
        
        // Face target
        if (m_pAnimComponent) {
            glm::vec2 direction = targetPosition - currentPosition;
            if (glm::length(direction) > 0.01f) {
                direction = glm::normalize(direction);
                
                std::string animationName;
                if (fabs(direction.x) > fabs(direction.y)) {
                    animationName = (direction.x > 0.0f) ? "StandEast" : "StandWest";
                } else {
                    animationName = (direction.y > 0.0f) ? "StandNorth" : "StandSouth";
                }
                
                if (m_pAnimComponent->GetCurrentAnimation()->m_strName != animationName) {
                    m_pAnimComponent->SetAnimation(animationName);
                    m_pAnimComponent->SetOriginToCenterOfFrame();
                }
            }
        }
        return;
    }
    
    // Move towards target
    glm::vec2 direction = glm::vec2(0.0f);
    if (distanceToPlayer > 0.01f) {
        direction = (targetPosition - currentPosition) / distanceToPlayer;
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
    } else {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

void HarpyController::HandleIdleState(float delta) {
    const float distanceToPlayer = glm::length(
        m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - 
        m_pTransform->GetGlobalPosition());
    
    if (distanceToPlayer <= m_detectionRange) {
        SetEmote(EnemyEmote::EXCLAMATION);
        ChangeState(EnemyState::CHASING);
    }
}

void HarpyController::HandleChasingState(float delta) {
    MoveTowardsTarget(delta);
    
    const float distanceToPlayer = glm::length(
        m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - 
        m_pTransform->GetGlobalPosition());
    
    if (distanceToPlayer <= m_rangedRange && m_rangedTimer <= 0.0f && 
        m_transitionTimer.Elapsed() >= m_transitionDelay && IsTargetInLOS()) {
        ChangeState(EnemyState::ATTACKING);
    }
}

void HarpyController::HandleAttackingState(float delta) {
    if (!m_pTarget) {
        ChangeState(EnemyState::IDLE);
        return;
    }

    // Handle windup
    if(m_rangedWindupTimer > 0.0f) {
        m_rangedWindupTimer -= delta;
        
        // Clamp to zero to ensure one execution
        if (m_rangedWindupTimer <= 0.0f) {
            m_rangedWindupTimer = 0.0f;
        }

        // Visual feedback - glow effect
        float normalizedWindup = 1.0f - (m_rangedWindupTimer / m_rangedWindupTime);
        normalizedWindup = glm::clamp(normalizedWindup, 0.0f, 1.0f);
        
        glm::vec3 glowColor;
        switch (m_currentAttackPattern) {
            case AttackPattern::SINGLE:
                glowColor = glm::vec3(1.0f, 0.5f + (normalizedWindup * 0.5f), 0.0f);
                break;
            case AttackPattern::SPREAD:
                glowColor = glm::vec3(0.8f, 0.0f, 0.8f + (normalizedWindup * 0.2f));
                break;
            case AttackPattern::BURST:
                glowColor = glm::vec3(1.0f, 0.0f, 0.0f + (normalizedWindup * 0.5f));
                break;
        }
        
        m_pAnimComponent->SetTint(glowColor);
        
        // Update facing direction
        const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
        const float distanceToPlayer = glm::length(targetPosition - currentPosition);
        glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
        
        std::string animationName;
        if (fabs(direction.x) > fabs(direction.y)) {
            animationName = (direction.x > 0.0f) ? "StandEast" : "StandWest";
        } else {
            animationName = (direction.y > 0.0f) ? "StandNorth" : "StandSouth";
        }
        
        if (m_pAnimComponent->GetCurrentAnimation()->m_strName != animationName) {
            m_pAnimComponent->SetAnimation(animationName);
            m_pAnimComponent->SetOriginToCenterOfFrame();
        }
        
        // Cancel attack if target moved too far
        if (distanceToPlayer > m_rangedRange * 1.5f) {
            m_pAnimComponent->SetTint(glm::vec3(1.0f));
            m_rangedTimer = m_rangedCooldown * 0.5f;
            m_attackChain = 0;
            m_rangedWindupTimer = m_rangedWindupTime;
            ChangeState(EnemyState::CHASING);
            return;
        }
    }
    // Execute attack
    else if (m_rangedWindupTimer <= 0.0f) {
        m_pAnimComponent->SetTint(glm::vec3(1.0f));
        
        // Execute attack pattern
        switch (m_currentAttackPattern) {
            case AttackPattern::SINGLE: PerformSingleShot(); break;
            case AttackPattern::SPREAD: PerformSpreadShot(); break;
            case AttackPattern::BURST: PerformBurstAttack(); break;
        }
        
        wolf::Audio::Play("data/sounds/sfx_fireball_shot.wav", 0.7f, 0.0f, 0.0f, true);
        
        m_attackChain--;
        
        if (m_attackChain > 0) {
            m_rangedWindupTimer = m_rangedWindupTime * 0.4f;
        } else {
            m_rangedTimer = m_rangedCooldown;
            m_rangedWindupTimer = m_rangedWindupTime;
            ChangeState(EnemyState::CHASING);
        }
    }
}

void HarpyController::HandlePetrifiedState(float delta) {
    ChangeState(EnemyState::DEATH);
}

void HarpyController::HandleStunnedState(float delta) {
    if(m_stunnedTimer >= m_stunnedTime) {
        if (m_last_state == EnemyState::IDLE) {
            SetEmote(EnemyEmote::EXCLAMATION);   
        }
        ChangeState(EnemyState::CHASING);
    } else {
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
        m_stunnedTimer += delta;
    }
}

void HarpyController::UpdateAnimationBasedOnDirection() {
    if (!m_pAnimComponent || !m_pVelocity) return;

    glm::vec2 velocity = m_pVelocity->GetVelocity();
    
    // Smooth velocity for animation
    m_smoothedVelocity = m_smoothedVelocity * (1.0f - ANIMATION_SMOOTHING_FACTOR) + 
                        velocity * ANIMATION_SMOOTHING_FACTOR;
    
    // Only update if moving
    if (glm::length(m_smoothedVelocity) > 0.01f) {
        std::string animationName;
        
        if (fabs(m_smoothedVelocity.x) > fabs(m_smoothedVelocity.y)) {
            animationName = (m_smoothedVelocity.x > 0.0f) ? "StandEast" : "StandWest";
        } else {
            animationName = (m_smoothedVelocity.y > 0.0f) ? "StandNorth" : "StandSouth";
        }

        if (m_pAnimComponent->GetCurrentAnimation()->m_strName != animationName) {
            m_pAnimComponent->SetAnimation(animationName);
            m_pAnimComponent->SetOriginToCenterOfFrame();
        }
    }
}

void HarpyController::HandleDeathState(float delta) {
    // Fall over
    if(m_fallDeadTimer <= m_timeToFallDead) {
        if(m_fallDeadTimer == 0.0f) {
            if (m_pVelocity) {
                m_pVelocity->SetVelocity(glm::vec2(0.0f));
            }

            ColliderComponent* collider = GetGameObject()->GetComponent<ColliderComponent>();
            if(collider) {
                collider->SetIgnoreTag(m_uiPlayerGOId);
            }
            
            m_pAnimComponent->SetTint(glm::vec3(1,0,0));
        }

        float angle = (90.0f / m_timeToFallDead) * delta;
        m_pTransform->RotateDegrees(angle);
        
        m_fallDeadTimer += delta;
    }
    // Lie dead
    else if(m_lieDeadTimer >= m_timeToLieDead) {
        // Spawn loot
        std::vector<wolf::GameObject*> pItemDrops = ItemDropCreator::Instance()->CreateItemDropFromLootTable(
            "data/minitaur_loot.yaml", m_pTransform->GetGlobalPosition(), -1.0f);
        
        // Push loot out of walls
        for (auto& pItem : pItemDrops) {
            VelocityComponent* pItemVel = pItem->GetComponent<VelocityComponent>();
            if (pItemVel) {
                pItemVel->ApplyKnockback(glm::vec2(1.0f, 0.0f), 10.0f);
            }
            ColliderComponent* pItemCollider = pItem->GetComponent<ColliderComponent>();
            if (pItemCollider) {
                pItemCollider->SetActive(false);
            }
        }

        // Delete harpy
        GetGameObject()->Delete();
    }
    m_lieDeadTimer += delta;
}

void HarpyController::EnterAttackState() {
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
    
    // Set attack chain based on pattern
    switch (m_currentAttackPattern) {
        case AttackPattern::BURST: m_attackChain = 3; break;
        case AttackPattern::SINGLE: m_attackChain = 1; break;
        case AttackPattern::SPREAD: m_attackChain = m_RNG.NextInt(1, 2); break;
    }
    
    // Face target
    if (m_pTarget && m_pAnimComponent) {
        const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
        glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
        
        std::string animationName;
        if (fabs(direction.x) > fabs(direction.y)) {
            animationName = (direction.x > 0.0f) ? "StandEast" : "StandWest";
        } else {
            animationName = (direction.y > 0.0f) ? "StandNorth" : "StandSouth";
        }
        
        m_pAnimComponent->SetAnimation(animationName);
        m_pAnimComponent->SetOriginToCenterOfFrame();
    }
    
    // Set windup timer based on pattern
    switch (m_currentAttackPattern) {
        case AttackPattern::SINGLE: m_rangedWindupTimer = m_rangedWindupTime * 1.2f; break;
        case AttackPattern::SPREAD: m_rangedWindupTimer = m_rangedWindupTime; break;
        case AttackPattern::BURST: m_rangedWindupTimer = m_rangedWindupTime * 0.7f; break;
    }
    
    // Visual feedback
    if (m_pAnimComponent) {
        switch (m_currentAttackPattern) {
            case AttackPattern::SINGLE: m_pAnimComponent->SetTint(glm::vec3(1.0f, 0.5f, 0.0f)); break;
            case AttackPattern::SPREAD: m_pAnimComponent->SetTint(glm::vec3(0.8f, 0.0f, 0.8f)); break;
            case AttackPattern::BURST: m_pAnimComponent->SetTint(glm::vec3(1.0f, 0.0f, 0.0f)); break;
        }
    }
}

void HarpyController::EnterChasingState() {
    m_transitionTimer.Reset();
    m_transitionTimer.Start();
}

void HarpyController::EnterIdleState() {
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void HarpyController::EnterStunnedState() {
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
}

void HarpyController::EnterDeathState() {
    SetEmote(EnemyEmote::NONE);
}

void HarpyController::EnterPetrifiedState() {} // No specific action needed

void HarpyController::ExitAttackState() {
    if(m_pAnimComponent) {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }
    
    if (m_attackChain > 0) {
        m_rangedTimer = m_rangedCooldown * 0.7f;
        m_attackChain = 0;
    }
    
    m_rangedTimer = m_rangedCooldown;
    m_rangedWindupTimer = m_rangedWindupTime;
}

void HarpyController::ExitChasingState() {
    m_transitionTimer.Reset();
    m_transitionTimer.Stop();
    m_transitionDelay = m_RNG.NextFloat(0.8f, 1.6f);
}

void HarpyController::ExitStunnedState() {
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_stunnedTimer = 0.0f;
}

void HarpyController::ExitPetrifiedState() {
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_pAnimComponent->SetAnimPaused(false);
}

void HarpyController::ExitIdleState() {} // No specific action needed

void HarpyController::SetEmote(EnemyEmote p_emote) {
    m_fEmoteTimer = EMOTE_TIME;
    switch(p_emote) {
        case EnemyEmote::EXCLAMATION:
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("Exclamation");
            break;
        case EnemyEmote::QUESTION:
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("Question");
            break;
        case EnemyEmote::NONE:
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("None");
            break;
    }
    m_emote = p_emote;
}

void HarpyController::HandleInfighting(const InfightingEvent& event) {
    if (event.m_pVictim == GetGameObject()) {
        // Ignore if already attacking this enemy
        if (m_pTarget == event.m_pAttacker) return;

        // Switch target to the attacker
        m_pTarget = event.m_pAttacker;
        ChangeState(EnemyState::CHASING);
    }
}

void HarpyController::RevertBackToPlayer() {
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        m_pTarget = playerController.GetGameObject();
        return;
    }
    m_pTarget = nullptr;
}

void HarpyController::SetupCombatBehaviorTree() {
    auto root = std::make_unique<Selector>();
    
    // ATTACK
    auto attackSeq = std::make_unique<Sequence>();
    attackSeq->AddBehaviorNode(std::make_unique<ConditionNode>([this]() { 
        return GetDistanceToTarget() <= m_rangedRange && 
               m_rangedTimer <= 0.0f && 
               IsTargetInSight() && 
               IsTargetInLOS();
    }));
    attackSeq->AddBehaviorNode(std::make_unique<ActionNode>([this](float delta) {
        if (m_state != EnemyState::ATTACKING) {
            // Choose pattern based on coordination with nearby harpies
            int nearbyHarpies = g_blackboard.GetInt("nearbyHarpies");
            
            if (nearbyHarpies >= 2) {
                // Use ID for coordinated attack patterns
                int patternIndex = GetGameObject()->GetID() % 3;
                m_currentAttackPattern = static_cast<AttackPattern>(patternIndex);
            } else {
                // Solo harpy - weighted random selection
                int randomVal = m_RNG.NextInt(0, 100);
                
                if (randomVal < 40) {
                    m_currentAttackPattern = AttackPattern::SINGLE;
                } else if (randomVal < 75) {
                    m_currentAttackPattern = AttackPattern::SPREAD;
                } else {
                    m_currentAttackPattern = AttackPattern::BURST;
                }
            }
            
            ChangeState(EnemyState::ATTACKING);
            SetEmote(EnemyEmote::EXCLAMATION);
        }
        return (m_state == EnemyState::ATTACKING) ? BehaviorNode::Status::RUNNING : BehaviorNode::Status::SUCCESS;
    }));
    root->AddBehaviorNode(std::move(attackSeq));
    
    // CHASE
    auto chaseSeq = std::make_unique<Sequence>();
    chaseSeq->AddBehaviorNode(std::make_unique<ConditionNode>([this]() { 
        return IsTargetInSight() && GetDistanceToTarget() > m_rangedRange; 
    }));
    chaseSeq->AddBehaviorNode(std::make_unique<ActionNode>([this](float) {
        if (m_state != EnemyState::CHASING) ChangeState(EnemyState::CHASING);
        return BehaviorNode::Status::RUNNING;
    }));
    root->AddBehaviorNode(std::move(chaseSeq));
    
    m_combatBehaviorTree = std::make_unique<BehaviorTree>(std::move(root));
}

void HarpyController::EvaluateStrategy() {
    if (!m_pTarget || !m_pHealth) return;
    
    float healthPct = m_pHealth->GetHealth() / m_pHealth->GetMaxHealth() * 100.0f;
    int nearbyHarpies = g_blackboard.GetInt("nearbyHarpies");
    
    // Priority checks
    if (healthPct < 30.0f) {
        g_blackboard.SetStrategy(Strategy::DEFENSIVE);
        return;
    }
    
    auto* playerHealth = m_pTarget->GetComponent<HealthComponent>();
    if (playerHealth && playerHealth->GetHealth() < 20.0f) {
        g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
        return;
    }
    
    // Group vs solo
    if (nearbyHarpies >= 1) {
        // Assign roles by ID
        g_blackboard.SetStrategy(Strategy(GetGameObject()->GetID() % 3));
    } else {
        // Defensive if player attacking, otherwise aggressive
        g_blackboard.SetStrategy(
            m_pTarget->GetComponent<PlayerController>() && 
            m_pTarget->GetComponent<PlayerController>()->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING ?
            Strategy::DEFENSIVE : Strategy::AGGRESSIVE
        );
    }
}

void HarpyController::UpdateBlackboard() {
    if (!m_pTarget || !m_pTransform) return;
    
    // Update player distance and health
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    g_blackboard.SetFloat("playerDistance", glm::length(
        m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - selfPos));
    
    if (m_pHealth)
        g_blackboard.SetFloat("selfHealthPercentage", 
            m_pHealth->GetHealth() / m_pHealth->GetMaxHealth() * 100.0f);
    
    // Count nearby harpies periodically
    static float harpyCountTimer = 0.0f;
    if ((harpyCountTimer += 0.1f) > 0.5f) {
        int count = 0;
        for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<HarpyController>()) {
            if (controller.GetGameObject() != GetGameObject() && controller.GetTarget() == m_pTarget) {
                float dist = glm::length(controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - selfPos);
                if (dist < m_detectionRange * 1.5f) count++;
            }
        }
        g_blackboard.SetInt("nearbyHarpies", count);
        harpyCountTimer = 0.0f;
    }
}

bool HarpyController::IsTargetInSight() {
    if (!m_pTarget || !m_pTransform) return false;
    
    float distanceToTarget = glm::length(
        m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - 
        m_pTransform->GetGlobalPosition());
    
    return distanceToTarget <= m_detectionRange && IsTargetInLOS();
}

float HarpyController::GetDistanceToTarget() const {
    return (!m_pTarget || !m_pTransform) ? 99999.0f : glm::length(
        m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - 
        m_pTransform->GetGlobalPosition());
}

void HarpyController::PerformSingleShot() {
    if (!m_pTarget) return;
    
    auto& scene = GetGameObject()->GetScene();
    
    // Get positions
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
    
    // Create projectile
    auto& projectile = scene.CreateObject2D();
    
    // Status effects (longer burning for single shot)
    std::vector<std::pair<StatusComponent::StatusEffectType, float>> statusEffects = 
    {
        {StatusComponent::StatusEffectType::BURNING, 7.0f}
    };
    
    // Add components with higher damage for focused shot
    projectile.AddComponent<AttackDamageComponent>(
        m_baseDamage * 1.5f, m_pColliderManager, 0.0f, statusEffects, GetGameObject());
    
    auto& projectileSprite = projectile.AddComponent<AnimatedSprite2D>("data/fireball_anim_init.yaml");
    projectileSprite.SetLayer(20);
    projectileSprite.SetAnimation("Default");
    
    // Setup hitbox
    glm::vec2 projectileDimensions(10.0f, 10.0f);
    glm::vec2 hurtboxOffset(-5.0f, 5.0f);
    auto& projectileCollider = projectile.AddComponent<ColliderComponent>(
        ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
    projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
    projectileCollider.SetIgnoreTag(GetGameObject()->GetID());
    
    // Add tracking and destruction components
    projectile.AddComponent<HomingComponent>(m_pTarget, 8.0f, 0.2f);
    projectile.AddComponent<TimedDestroyerComponent>(10);
    
    // Set velocity and position
    auto& projectileVelocityComponent = projectile.AddComponent<VelocityComponent>();
    projectileVelocityComponent.SetVelocity(direction * 200.0f);
    projectile.GetComponent<wolf::Transform2D>()->SetPosition(m_pTransform->GetGlobalPosition() + direction * 15.0f);
    projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(4.0f));
    
    // Add the particle component for fire trail
    auto& particleComponent = projectile.AddComponent<ParticleComponent>(75);
    bool configLoaded = particleComponent.LoadConfigFromYAML("data/particles/fire_trail.yaml");
    if (!configLoaded) {
        wolf::Warning("Failed to load fire_trail.yaml for projectile");
        // Manual fallback setup would go here
    }
    
    // Set continuous emission to true and a reasonable rate
    particleComponent.SetContinuousEmission(true);
    particleComponent.SetEmissionRate(40.0f); // 40 particles per second

    // Add light
    wolf::GameObject* pLightGO = &scene.CreateObject2D();
    auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(1.0f, 0.64f, 0.0f, 0.75f), 50.0f, true);
    projectile.AddChild(*pLightGO);
    pLightComponent.Init();
}

void HarpyController::PerformSpreadShot() {
    if (!m_pTarget) return;
    
    auto& scene = GetGameObject()->GetScene();
    
    // Get positions and direction
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
    
    // Calculate perpendicular vector for spread
    glm::vec2 perpendicular(-direction.y, direction.x);
    
    // Create 5 projectiles in a spread pattern
    const int numProjectiles = 5;
    const float spreadWidth = 40.0f;
    
    for (int i = 0; i < numProjectiles; i++) {
        auto& projectile = scene.CreateObject2D();
        
        // Status effects
        std::vector<std::pair<StatusComponent::StatusEffectType, float>> statusEffects = 
        {
            {StatusComponent::StatusEffectType::BURNING, 3.0f}
        };
        
        // Add components with lower damage for spread shots
        projectile.AddComponent<AttackDamageComponent>(
            m_baseDamage * 0.8f, m_pColliderManager, 0.0f, statusEffects, GetGameObject());
        
        auto& projectileSprite = projectile.AddComponent<AnimatedSprite2D>("data/fireball_anim_init.yaml");
        projectileSprite.SetLayer(20);
        
        // Setup hitbox
        glm::vec2 projectileDimensions(10.0f, 10.0f);
        glm::vec2 hurtboxOffset(-5.0f, 5.0f);
        auto& projectileCollider = projectile.AddComponent<ColliderComponent>(
            ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
        projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
        projectileCollider.SetIgnoreTag(GetGameObject()->GetID());
        
        // Less tracking for spread shots
        projectile.AddComponent<HomingComponent>(m_pTarget, 2.0f, 0.05f);
        projectile.AddComponent<TimedDestroyerComponent>(8);
        
        // Calculate spread offset and angle variation
        float spreadOffset = (i - (numProjectiles - 1) / 2.0f) * (spreadWidth / (numProjectiles - 1));
        glm::vec2 offset = perpendicular * spreadOffset;
        
        float angleVariation = spreadOffset / spreadWidth * 0.3f;
        glm::vec2 adjustedDirection = glm::normalize(direction + perpendicular * angleVariation);
        
        // Set velocity and position
        auto& projectileVelocityComponent = projectile.AddComponent<VelocityComponent>();
        projectileVelocityComponent.SetVelocity(adjustedDirection * 168.0f);
        projectile.GetComponent<wolf::Transform2D>()->SetPosition(m_pTransform->GetGlobalPosition() + offset * 0.5f);
        projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(2.5f));
        
        // Add the particle component for fire trail
        auto& particleComponent = projectile.AddComponent<ParticleComponent>(75);
        bool configLoaded = particleComponent.LoadConfigFromYAML("data/particles/fire_trail.yaml");
        if (!configLoaded) {
            wolf::Warning("Failed to load fire_trail.yaml for projectile");
            // Manual fallback setup would go here
        }
        
        // Set continuous emission to true and a reasonable rate
        particleComponent.SetContinuousEmission(true);
        particleComponent.SetEmissionRate(40.0f); // 40 particles per second
        // Add light
        wolf::GameObject* pLightGO = &scene.CreateObject2D();
        auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(1.0f, 0.64f, 0.0f, 0.75f), 50.0f, true);
        projectile.AddChild(*pLightGO);
        pLightComponent.Init();
    }
}

void HarpyController::PerformBurstAttack() {
    if (!m_pTarget) return;
    
    auto& scene = GetGameObject()->GetScene();
    
    // Get positions and direction
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 harpyDirection = glm::normalize(targetPosition - currentPosition);
    
    // Calculate perpendicular for spread
    glm::vec2 perpendicularVector(-harpyDirection.y, harpyDirection.x);
    
    // Common projectile properties
    glm::vec2 projectileDimensions(10.0f, 10.0f);
    glm::vec2 hurtboxOffset(-5.0f, 5.0f);
    glm::vec2 projectileDefaultVelocity = harpyDirection * 168.0f;
    
    // Spawn 3 projectiles per volley
    for(int i = -1; i <= 1; i += 1) {
        auto& projectile = scene.CreateObject2D();
        
        // Status effects
        std::vector<std::pair<StatusComponent::StatusEffectType, float>> statusEffects = 
        {
            {StatusComponent::StatusEffectType::BURNING, 4.0f}
        };
        
        // Add components
        projectile.AddComponent<AttackDamageComponent>(
            m_baseDamage, m_pColliderManager, 0.0f, statusEffects, GetGameObject());
        
        auto& projectileSprite = projectile.AddComponent<AnimatedSprite2D>("data/fireball_anim_init.yaml");
        projectileSprite.SetLayer(20);
        
        auto& projectileCollider = projectile.AddComponent<ColliderComponent>(
            ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
        projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
        projectileCollider.SetIgnoreTag(GetGameObject()->GetID());
        
        projectile.AddComponent<HomingComponent>(m_pTarget, 6.0f, 0.1f);
        projectile.AddComponent<TimedDestroyerComponent>(10);
        
        auto& projectileVelocityComponent = projectile.AddComponent<VelocityComponent>();
        projectileVelocityComponent.SetVelocity(projectileDefaultVelocity);

        // Add the particle component for fire trail
        auto& particleComponent = projectile.AddComponent<ParticleComponent>(75);
        bool configLoaded = particleComponent.LoadConfigFromYAML("data/particles/fire_trail.yaml");
        if (!configLoaded) {
            wolf::Warning("Failed to load fire_trail.yaml for projectile");
            // Manual fallback setup would go here
        }
        
        // Set continuous emission to true and a reasonable rate
        particleComponent.SetContinuousEmission(true);
        particleComponent.SetEmissionRate(40.0f); // 40 particles per second
        
        // Position with spread
        glm::vec2 offset = perpendicularVector * (30.0f * i);
        projectile.GetComponent<wolf::Transform2D>()->SetPosition(m_pTransform->GetGlobalPosition() + offset);
        projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));

        // Add light
        wolf::GameObject* pLightGO = &scene.CreateObject2D();
        auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(1.0f, 0.64f, 0.0f, 0.75f), 50.0f, true);
        projectile.AddChild(*pLightGO);
        pLightComponent.Init();
    }
}

bool HarpyController::IsTargetInLOS() {
    if (!m_pTarget || !m_pTransform) return false;
    
    glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Use DDA calculator to check line of sight
    return DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos) == targetPos;
}