#pragma once
#include "TriggerEvent.h"
#include "W_EventManager.h"
#include "TriggerComponent.h"
#include <vector>

class TriggerManager {
public:
    void AddTrigger(TriggerComponent* trigger);
    void Update(float delta);
    
private:
    std::vector<TriggerComponent*> m_triggers;
};

