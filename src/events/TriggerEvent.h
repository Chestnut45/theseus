#pragma once

#include "W_GameObject.h"
#include <string>

// Struct to represent a trigger event, similar to other event definitions
struct TriggerEvent {
    // Constructor
    TriggerEvent(const std::string& name, wolf::GameObject* object)
        : triggerName(name), triggerObject(object) {}

    // Name of the trigger event
    std::string triggerName;      
    // Pointer to the object that triggered the event
    wolf::GameObject* triggerObject;
};
