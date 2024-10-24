#pragma once

#include "W_GameObject.h"

struct TriggerEvent
{
    TriggerEvent(const std::string& name, wolf::GameObject* object)
        : triggerName(name), triggerObject(object) {}

    std::string triggerName;      // Name of the trigger event
    wolf::GameObject* triggerObject; // Reference to the object that triggered the event
};
