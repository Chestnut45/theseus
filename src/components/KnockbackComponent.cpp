#include "KnockbackComponent.h"

void KnockbackComponent::ApplyKnockback(const glm::vec2& direction, float force, float duration) {
    forceDirection = glm::normalize(direction);
    forceMagnitude = force;
    this->duration = duration;
    elapsedTime = 0.0f;
    isActive = true;
}

void KnockbackComponent::Update(float delta) {
    if (!isActive) return;

    elapsedTime += delta;
    if (elapsedTime >= duration) {
        Reset();
    }
}

void KnockbackComponent::Reset() {
    forceDirection = glm::vec2(0.0f);
    forceMagnitude = 0.0f;
    duration = 0.0f;
    elapsedTime = 0.0f;
    isActive = false;
}
