#pragma once

class TriggerComponent;

// Event that is sent when a trigger's purpose is finished
struct TriggerPurposeFinishedEvent {
    TriggerComponent* m_pTrigger = nullptr;

    TriggerPurposeFinishedEvent(TriggerComponent* trigger)
        : m_pTrigger(trigger) {}
};
