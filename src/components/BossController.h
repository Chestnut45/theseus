#pragma once

#include <W_BaseComponent.h>

// Forward declarations
class VelocityComponent;
class AnimatedSprite2D;
class HealthComponent;
class StatusComponent;
class ColliderComponent;
class PlayerController;

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

        // Phase 3 states
        SEARCHING,
        FIRE_BREATH_ATTACK,
        CHARGE_ATTACK,

        // Special states
        TAUNT, // Could play an animation when the player dies
        DEAD
    };

    BossController();
    ~BossController();
    
    // Initialize the boss controller
    // POST: All member component pointers are valid OR an error is logged
    void Init();

    void Update(float delta);

    void SetActive(bool active) { m_active = active; }
    bool IsActive() const { return m_active; }

private:

    // State information
    FightPhase m_phase = FightPhase::PHASE_1;
    State m_state = State::SIT;

    // Boss object component pointers
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    AnimatedSprite2D* m_pAnimSprite = nullptr;
    HealthComponent* m_pHealth = nullptr;
    StatusComponent* m_pStatus = nullptr;
    ColliderComponent* m_pCollider = nullptr;

    // Cached player references
    wolf::GameObject* m_pPlayerObject = nullptr;
    PlayerController* m_pPlayerController = nullptr;

    // NOTE: All stats are initialized in Init() so changes only cause a single file to recompile

    // General stats
    bool m_active;
    int m_maxHealth;
    
    // Phase 1 stats
    int m_throneBlockRange;
    int m_numSummons;

    // Phase 2 stats
    int m_axeAttackDamage;
    int m_axePunishDamage;
    int m_minDistToPlayer;
    int m_maxDistToPlayer;

    // Phase 3 stats
    int m_fireBreathDamage;
    int m_fireBreathRange;
    int m_chargeAttackDamage;
    int m_chargeAttackRange;
    int m_stunTime;

    float m_searchSpeed;
    float m_searchTimer;

    // Updates the animated sprite based on state,
    // regardless of what phase of the fight we're in
    void UpdateAnimation();

    // Phase 1 methods
    void EnterPhase1();
    void UpdatePhase1(float delta);
    void SummonMinitaur();
    void BlockPlayerAttack();

    // Phase 2 methods
    void EnterPhase2();
    void UpdatePhase2(float delta);
    void StartAxeAttack();
    void DodgePlayerAttack();

    // Phase 3 methods
    void EnterPhase3();
    void UpdatePhase3(float delta);
    void ChangeStatesPhase3(State p_state);

    void StartSearch();
    void Search(float delta);
    void EndSearch();
    void MoveTowardsPlayer(float delta);

    void StartFireBreathAttack();
    void StartChargeAttack();
};