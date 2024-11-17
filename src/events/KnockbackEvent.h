// KnockbackEvent.h

#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

struct KnockbackEvent {
    wolf::GameObject* targetObject = nullptr;  // The object to apply knockback to
    glm::vec2 knockbackDirection;              // Direction of the knockback
    float knockbackForce = 0.0f;               // Magnitude of the knockback

    // Constructor for initializing event data
    KnockbackEvent(wolf::GameObject* pTargetObject, const glm::vec2& direction, float force)
        : targetObject(pTargetObject), knockbackDirection(direction), knockbackForce(force) {}
};
