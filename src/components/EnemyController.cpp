#include "EnemyController.h"
#include <cassert>

EnemyController::EnemyController() = default;

EnemyController::~EnemyController()
{
    // Clean up pointers but avoid deleting objects (managed by GameObject)
    m_pTransform = nullptr;
    m_pHealth = nullptr;
    m_pCollider = nullptr;
}

void EnemyController::Init()
{
    auto* pGameObject = GetGameObject();
    if (pGameObject)
    {
        // Initialize shared components that all enemies need
        m_pTransform = pGameObject->GetComponent<wolf::Transform2D>();
        m_pHealth = pGameObject->GetComponent<HealthComponent>();
        m_pCollider = pGameObject->GetComponent<ColliderComponent>();

        // Ensure all the necessary components are initialized
    }
}

void EnemyController::Update(float delta)
{
    // General state update logic (but nothing specific to movement, animations, or attacks)
    switch (m_state)
    {
        case EnemyState::IDLE:
            // Concrete enemies will implement their own idle behavior
            break;
        case EnemyState::CHASING:
            // Concrete enemies will implement their own chasing behavior
            break;
        case EnemyState::ATTACKING:
            // Concrete enemies will implement their own attacking behavior
            break;
        case EnemyState::DEATH:
            HandleDeathState();
            break;
    }
}

void EnemyController::SetColliderManager(ColliderManager* pColliderManager)
{
    m_pColliderManager = pColliderManager;
}

ColliderManager* EnemyController::GetColliderManager() const
{
    return m_pColliderManager;
}

void EnemyController::ChangeState(EnemyState newState)
{
    m_state = newState;
}

void EnemyController::HandleDeathState()
{
    // General behavior when an enemy enters the death state
    if (m_pHealth && m_pHealth->GetHealth() <= 0)
    {
        GetGameObject()->Delete(); // Destroy the enemy game object
    }
}