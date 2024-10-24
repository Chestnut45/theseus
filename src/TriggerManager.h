#pragma once
#include <vector>
#include "TriggerComponent.h"

class TriggerManager {
public:
    void AddTrigger(TriggerComponent* trigger) {
        m_triggers.push_back(trigger);
    }

    void Update(float delta) {
        for (auto* trigger : m_triggers) {
            if (trigger != nullptr) {
                trigger->Update(delta);  // Pass delta to each trigger
            }
        }
    }

private:
    std::vector<TriggerComponent*> m_triggers;
};
