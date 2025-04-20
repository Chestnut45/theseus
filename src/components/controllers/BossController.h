#pragma once
//-----------------------------------------------------------------------------
// File: BossController.h
// Original Author:	D'Anyil Landry
// Modifications: Aurora Ryder, Nguyễn Minh Nhật, Youssef Ashraf
// Controls the  behaviours of the Boss - NOT inherited from EnemyController
//-----------------------------------------------------------------------------
#include <W_BaseComponent.h>
#include <glm/glm.hpp>
#include <W_RNG.h>

#include <glm/vec2.hpp>
#include <W_Timer.h>
#include <events/DamageEvent.h>
#include <unordered_set>

// Forward declarations
class VelocityComponent;
class AnimatedSprite2D;
class HealthComponent;
class StatusComponent;
class ColliderComponent;
class HomingComponent;
class PlayerController;
class LabyrinthManager;
class LightComponent;

// NOTE: You can only forward declare from within the same namespace
namespace wolf
{
    class Transform2D;
}

// Component type to manage creation and simulation of the Minotaur final boss
class BossController : public wolf::BaseComponent
{
public:

    enum struct FightPhase
    {
        PHASE_1,
        PHASE_2,
        PHASE_3
    };

    enum struct State
    {
        // Phase 1 states
        SIT,
        SUMMONING,
        DEFLECT,

        // Phase 2 states
        APPROACH,
        STRAFE,
        DODGE,
        AXE_ATTACK,
        LEAP_ATTACK,
        TRANSITION_TO_PHASE_3,

        // Phase 3 states
        SEARCHING,
        FIRE_BREATH_ATTACK,
        CHARGE_ATTACK,
        IDLE,
        PULL,
        STUNNED,

        // Special states
        TAUNT, // Could play an animation when the player dies
        DEAD
    };

    BossController();
    ~BossController();
    
    // Initialize the boss controller
    // POST: All member component pointers are valid OR an error is logged
    void Init();

    // Call to clear internal state and delete objects from the scene
    void Deinit();

    void Update(float delta);

    void SetActive(bool active) { m_active = active; }
    bool IsActive() const { return m_active; }

    // Starts the bossfight (does not spawn first wave!)
    void StartBossfight();

    bool IsAirborne() const;
    bool IsAlive() const;

private:

    // State information
    FightPhase m_phase = FightPhase::PHASE_1;
    State m_state = State::SIT;
    State m_prevState = State::SIT;

    // Boss object component pointers
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    AnimatedSprite2D* m_pAnimSprite = nullptr;
    HealthComponent* m_pHealth = nullptr;
    StatusComponent* m_pStatus = nullptr;
    ColliderComponent* m_pCollider = nullptr;
    HomingComponent* m_pHoming = nullptr;   // Added by Nhật
    LightComponent* m_pLight = nullptr;

    // Pointer to shadow sprite (only valid during phase 2!)
    wolf::GameObject* m_pShadowObject = nullptr;

    // Pointer to game object that contains all pillar objects as child objects
    wolf::GameObject* m_pBossPillarGroup = nullptr;

    // Cached player references
    wolf::GameObject* m_pPlayerObject = nullptr;
    PlayerController* m_pPlayerController = nullptr;

    //labyrinth reference (needed for spawns)
    LabyrinthManager* m_pLabyrinthManager = nullptr;

    // Utility members
    wolf::RNG m_rng;

    // NOTE: All stats are initialized in Init() so changes only cause a single file to recompile

    // General stats
    bool m_active;
    bool m_renderHealthBar = false;
    int m_maxHealth;
    float m_prevHealthFraction;
    wolf::Timer m_damageFlashTimer;
    glm::vec2 m_centerOfChamber;
    glm::vec2 m_bottomLeftCorner;
    glm::vec2 m_topRightCorner;
    
    // Phase 1 stats
    int m_throneBlockRange;
    int m_numSummons;
    float m_forcefieldRadius;  // Defines the range where the forcefield affects the player
    float m_slowdownFactor;      // Reduces the player's velocity when inside the forcefield

    int m_currentWave;
    int m_remainingEnemies;
    float m_waveTransitionTimer;
    bool m_waveActive;
    std::unordered_set<int> m_enemyIDs;
    


    // Phase 2 stats
    int m_attackChain;
    int m_prevAttack;
    int m_axeAttackDamage;
    int m_slamAttackDamage;
    int m_minDistToPlayer;
    int m_maxDistToPlayer;
    bool m_strafeClockwise;
    bool m_axeSummoned;
    bool m_slamStun;
    float m_strafeSpeed;
    float m_chaseSpeed;
    float m_shadowDistance;
    float m_altitude;
    wolf::Timer m_whooshTimer;
    wolf::Timer m_dodgeTimer;
    wolf::Timer m_strafeSwapTimer;
    wolf::Timer m_nextAttackTimer;
    wolf::Timer m_axeAttackTimer;
    wolf::Timer m_leapAttackTimer;
    wolf::Timer m_transitionTimer;
    wolf::Timer m_throneBreakTimer;
    glm::vec2 m_dodgeDir;
    glm::vec2 m_transitionStartPos;
    ColliderComponent* m_pAxeCollider = nullptr;

    // Phase 3 stats
    wolf::Timer m_fireSFXTimer;
    wolf::Timer m_deathTimer;
    float m_fireBreathWindupTime;   // Fire breath state members
    float m_fireBreathWindupTimer;
    glm::vec3 m_fireBreathWindupTint;
    int m_fireBreathDamage;     
    float m_fireBreathRange;
    float m_fireBreathRangeExtender;
    float m_fireBreathDuration;
    float m_fireBreathTimer;
    float m_fireBreathTurningCapRadian;
    float m_fireBreathTurningDelay;
    float m_fireBreathTurningTimer;
    bool m_fireBreathSFXPlayed = false;
    glm::vec2 m_lastDirection;

    int m_chargeAttackDamage;       // Charge state members
    int m_chargeAttackRange;
    float m_chargeWindupTime;
    float m_chargeWindupTimer;
    glm::vec3 m_chargeWindupTint;
    float m_chargeTurningCapDegree;
    float m_chargeTurningDelay;
    float m_chargeSpeed;
    float m_chargeKnockbackForce;
    int m_chargeChainCount;
    wolf::Timer m_chargeStompSFXTimer;

    float m_pullTime;               // Pull state members
    float m_pullTimer;
    float m_pullForce;

    float m_stunTime;               // Stun state members
    float m_stunTimer;              

    float m_searchSpeed;            // Search state members
    float m_searchTimer;
    float m_redirectTime;
    float m_redirectTimer;
    float m_redirectSpeedLimit;

    float m_idleTimer;              // Idle state members
    glm::vec2 m_idleTimeRange;

    float m_autoAttackRange;        // Misc.
    float m_superchargeHealthFraction;
    float m_isSupercharged;

    // Updates the animated sprite based on state,
    // regardless of what phase of the fight we're in
    void UpdateAnimation();

    void RenderHealthBar(float delta);

    // Handlers
    void OnDamageEvent(const DamageEvent& event);

    // Phase 1 methods
    void EnterPhase1();
    void UpdatePhase1(float delta);
    void HandleForcefield(float delta);
    void HandleKnockBackCollision(float delta);
    void StartWave();
    void SpawnWave(int waveIndex);
    void CheckWaveProgress(float delta);
    void CheckEnemyWaveHealth();
    glm::vec2 GetRandomValidSpawnPosition();
    void RenderImGui();
    bool IsValidSpawnTile(glm::ivec2 tilePos);
    void CleanupPhase1();


    // Phase 2 methods
    void EnterPhase2();
    void UpdatePhase2(float delta);
    void StartAxeAttack();
    void StartLeapAttack();
    void DodgePlayerAttack(const glm::vec2& dirToPlayer);

    // Phase 3 methods
    void EnterPhase3();
    void UpdatePhase3(float delta);

    void ChangeStatesPhase3(State p_state);

    void StartSearch();
    void Search(float delta);
    void EndSearch();
    void MoveTowardsPlayer(float delta);

    void StartIdle();
    void Idle(float delta);
    void EndIdle();

    void StartStunned();
    void Stunned(float delta);

    void StartFireBreathAttack();
    void AttackFireBreath(float delta);
    void BreatheFire(float delta);
    void BreatheFireSupercharged(float delta);
    void TurnToPlayer(float delta);

    void StartChargeAttack();
    void AttackCharge(float delta);
    void EndChargeAttack();

    void StartPull();
    void Pull(float delta);

    void Dead(float delta);

    void LastStandSupercharge();
};