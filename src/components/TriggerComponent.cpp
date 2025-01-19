#include "TriggerComponent.h"
#include "TriggerEvent.h"
#include "W_GameObject.h"
#include "PlayerController.h"
#include "MinitaurController.h"
#include "GorgonController.h"
#include "HarpyController.h"
#include "W_EventManager.h"

TriggerComponent::TriggerComponent(ColliderManager* colliderManager, TriggerType type, TriggerPurpose purpose, EntityListenType::type entityTypes)
    : m_colliderManager(colliderManager), m_triggerType(type), m_purpose(purpose), m_entityTypes(entityTypes), m_triggered(false) {

    wolf::EventManager::AddListener<TriggerPurposeFinishedEvent, TriggerComponent, &TriggerComponent::OnPurposeFinished>(*this);
}

TriggerComponent::~TriggerComponent() {
    wolf::EventManager::RemoveListener<TriggerPurposeFinishedEvent, TriggerComponent, &TriggerComponent::OnPurposeFinished>(*this);
}

void TriggerComponent::Update(float delta) {
    if (m_active && !m_triggered && (CheckPlayerCollision(delta) || CheckEnemyCollision(delta))) {
        m_triggered = true;

        // Dispatch the TriggerEvent with trap type information
        wolf::EventManager::TriggerEvent(TriggerEvent(GetGameObject(), m_triggerType, m_purpose));

        // Delete for single-use triggers
        if (m_triggerType == TriggerType::SINGLE_USE) {
            GetGameObject()->Delete();
        }
    }
}

bool TriggerComponent::CheckPlayerCollision(float delta)
{
    // Early out if not listening for player
    if (!(m_entityTypes & EntityListenType::PLAYER || m_entityTypes & EntityListenType::PLAYER_IGNORE_ROLLING)) return false;

    auto* plateGameObject = GetGameObject();
    if (!plateGameObject) {
        wolf::Error("TriggerComponent: GameObject is null!");
        return false;
    }

    auto* plateCollider = plateGameObject->GetComponent<ColliderComponent>();
    if (!plateCollider) {
        wolf::Error("TriggerComponent: Plate collider is null!");
        return false;
    }

    for (auto&& [_, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {

        // Ignore player if rolling
        if (m_entityTypes & EntityListenType::PLAYER_IGNORE_ROLLING &&
            playerController.GetPlayerAction() == PlayerController::PlayerAction::ROLLING)
        {
            continue;
        }
        
        auto* playerCollider = playerController.GetGameObject()->GetComponent<ColliderComponent>();
        if (playerCollider && m_colliderManager->IsColliding(*plateCollider, *playerCollider, delta)) {
            return true;
        }
    }
    return false;
}

bool TriggerComponent::CheckEnemyCollision(float delta)
{
    auto* plateGameObject = GetGameObject();
    if (!plateGameObject) {
        wolf::Error("TriggerComponent: GameObject is null!");
        return false;
    }

    auto* plateCollider = plateGameObject->GetComponent<ColliderComponent>();
    if (!plateCollider) {
        wolf::Error("TriggerComponent: Plate collider is null!");
        return false;
    }

    // Minitaurs
    if (m_entityTypes & EntityListenType::MINITAUR)
    {
        for (auto&& [_, minitaur] : GetGameObject()->GetScene().Each<MinitaurController>())
        {
            auto* pCollider = minitaur.GetGameObject()->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_colliderManager->IsColliding(*plateCollider, *pCollider, delta))
            {
                return true;
            }
        }
    }

    // Gorgons
    if (m_entityTypes & EntityListenType::GORGON)
    {
        for (auto&& [_, gorgon] : GetGameObject()->GetScene().Each<GorgonController>())
        {
            auto* pCollider = gorgon.GetGameObject()->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_colliderManager->IsColliding(*plateCollider, *pCollider, delta))
            {
                return true;
            }
        }
    }

    // Harpies
    if (m_entityTypes & EntityListenType::HARPY)
    {
        for (auto&& [_, harpy] : GetGameObject()->GetScene().Each<HarpyController>())
        {
            auto* pCollider = harpy.GetGameObject()->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_colliderManager->IsColliding(*plateCollider, *pCollider, delta))
            {
                return true;
            }
        }
    }

    return false;
}

void TriggerComponent::OnPurposeFinished(const TriggerPurposeFinishedEvent& event) {
    if (m_triggerType == TriggerType::REUSABLE && m_triggered && event.m_pTrigger == this) {
        m_triggered = false;
    }
}
