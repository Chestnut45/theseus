#include "TriggerComponent.h"
#include "TriggerEvent.h"
#include "W_GameObject.h"
#include "PlayerController.h"
#include "W_EventManager.h"

TriggerComponent::TriggerComponent(ColliderManager* colliderManager, TriggerType type, TriggerPurpose purpose)
    : m_colliderManager(colliderManager), m_triggerType(type), m_purpose(purpose), m_triggered(false) {
    if (purpose == TriggerPurpose::SPIKE_TRAP && type == TriggerType::REUSABLE) {
        wolf::EventManager::AddListener<TrapDestroyedEvent, TriggerComponent, &TriggerComponent::OnTrapDestroyed>(*this);
    }

    if (purpose == TriggerPurpose::BOULDER_TRAP && type == TriggerType::REUSABLE) {
        wolf::EventManager::AddListener<BoulderDestroyedEvent, TriggerComponent, &TriggerComponent::OnBoulderDestroyed>(*this);
    }
}

TriggerComponent::~TriggerComponent() {
    wolf::EventManager::RemoveListener<TrapDestroyedEvent, TriggerComponent, &TriggerComponent::OnTrapDestroyed>(*this);
    wolf::EventManager::RemoveListener<BoulderDestroyedEvent, TriggerComponent, &TriggerComponent::OnBoulderDestroyed>(*this);
}

void TriggerComponent::Update(float delta) {
    if (!m_triggered && CheckPlayerCollision(delta)) {
        m_triggered = true;

        // Dispatch the TriggerEvent with trap type information
        wolf::EventManager::TriggerEvent(TriggerEvent(GetGameObject(), m_triggerType, m_purpose));

        // Delete for single-use triggers
        if (m_triggerType == TriggerType::SINGLE_USE) {
            GetGameObject()->Delete();
        }
    }
}

bool TriggerComponent::CheckPlayerCollision(float delta) {
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
        auto* playerCollider = playerController.GetGameObject()->GetComponent<ColliderComponent>();
        if (playerCollider && m_colliderManager->IsColliding(*plateCollider, *playerCollider, delta)) {
            return true;
        }
    }
    return false;
}

void TriggerComponent::ResetTrigger() {
    if (m_triggerType == TriggerType::REUSABLE && m_triggered) {
        m_triggered = false;
        // wolf::Log("TriggerComponent: Trigger reset.");
    }
}

void TriggerComponent::OnTrapDestroyed(const TrapDestroyedEvent& event) {
    ResetTrigger();
}

void TriggerComponent::OnBoulderDestroyed(const BoulderDestroyedEvent& event) {
    ResetTrigger();
}
