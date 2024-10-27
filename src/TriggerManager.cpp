#include "TriggerManager.h"

void TriggerManager::AddTrigger(TriggerComponent* trigger) {
    m_triggers.push_back(trigger);
}

void TriggerManager::Update(float delta) {
    for (auto* trigger : m_triggers) {
        auto* triggerComponent = static_cast<TriggerComponent*>(trigger);
        triggerComponent->Update(delta);

        // Check if the trigger has been activated but the trap has not yet been spawned
        if (triggerComponent->IsTriggered() && !triggerComponent->IsTrapSpawned()) {
            // Trigger the event only when the trap hasn't been spawned
            wolf::EventManager::TriggerEvent(TriggerEvent("PressurePlateSteppedOn", triggerComponent->GetGameObject()));

            // Set the trap as spawned to prevent further triggering
            triggerComponent->SetTrapSpawned(true); // THIS IS HANDLED ONLY HERE
        }
    }
}
