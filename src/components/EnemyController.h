#pragma once

#include <wolf.h>
#include <components/HealthComponent.h>
#include <components/ColliderComponent.h>

// Generalized EnemyController that handles core state management and shared properties for all enemies
class EnemyController : public wolf::BaseComponent
{
public:
    enum class EnemyState
    {
        IDLE,
        PROSPECT,
        CHASING,
        ATTACKING,
        PETRIFIED,
        STUNNED,
        DEATH
    };
    EnemyController() = default;    
    ~EnemyController() = default; 

    virtual void Init();                  // General initialization of components
    virtual void Update(float delta);     // Update enemy state, to be extended in concrete enemies
    void SetColliderManager(ColliderManager* pColliderManager);  // Set the ColliderManager, general for all enemies
    ColliderManager* GetColliderManager() const;
    void ChangeState(EnemyState newState); // General state transition logic shared by all enemies
    

protected:
    // These components are common to all enemies and will be initialized here, but used in specific enemy classes
    wolf::Transform2D* m_pTransform = nullptr;
    HealthComponent* m_pHealth = nullptr;
    ColliderComponent* m_pCollider = nullptr;

    EnemyState m_state = EnemyState::IDLE;
    ColliderManager* m_pColliderManager = nullptr;
    wolf::GameObject* m_pTarget = nullptr;  // Target (usually the player)

    float m_fCountdownToDeath = 2.0f;
};