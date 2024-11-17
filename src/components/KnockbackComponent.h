#pragma once
#include "W_BaseComponent.h"
#include <glm/vec2.hpp>
#include <glm/glm.hpp>

class KnockbackComponent : public wolf::BaseComponent {
public:
    glm::vec2 forceDirection = glm::vec2(0.0f);
    float forceMagnitude = 0.0f;
    float duration = 0.0f;
    float elapsedTime = 0.0f;
    bool isActive = false;

    void ApplyKnockback(const glm::vec2& direction, float force, float duration);
    void Update(float delta);
    void Reset();
};