#include "EnemyController.h"
#include <cassert>
#include <LabyrinthManager.h>


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
    // Update chunk
    for (auto&&[_, lm] : GetGameObject()->GetScene().Each<LabyrinthManager>())
    {
        glm::ivec2 newChunkID = lm.GetChunkID(m_pTransform->GetGlobalPosition());
        if (m_chunkID != newChunkID)
        {
            wolf::GameObject* pChunk = lm.GetChunk(newChunkID);
            if (pChunk)
            {
                pChunk->AddChild(*GetGameObject());
                m_chunkID = newChunkID;
                wolf::Log("YEUP");
            }
        }
        break;
    }
    
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


void EnemyController::SetPlayerID(wolf::GameObjectID p_uiGOId) {
    m_uiPlayerGOId = p_uiGOId;
}

wolf::GameObjectID const EnemyController::GetPlayerID() {
    return m_uiPlayerGOId;
}