#include "BoulderTrapComponent.h"
#include "W_GameObject.h"
#include "W_EventManager.h"
#include "W_Transform2D.h"
#include "VelocityComponent.h"

BoulderTrapComponent::BoulderTrapComponent(ColliderManager* colliderManager, BoulderDirection direction, float speed, float lifespan)
    : m_colliderManager(colliderManager), m_direction(direction), m_speed(speed), m_lifespan(lifespan), m_timerStarted(false) {}

void BoulderTrapComponent::Update(float delta) {
    // Start the lifespan timer only when the boulder starts moving
    if (!m_timerStarted) {
        m_lifespanTimer.Start();

        // Set initial velocity
        auto* velocity = GetGameObject()->GetComponent<VelocityComponent>();
        if (velocity) {
            glm::vec2 initialVelocity(0.0f);
            switch (m_direction) {
                case BoulderDirection::UP:    initialVelocity.y = -m_speed; break;
                case BoulderDirection::DOWN:  initialVelocity.y = m_speed;  break;
                case BoulderDirection::LEFT:  initialVelocity.x = -m_speed; break;
                case BoulderDirection::RIGHT: initialVelocity.x = m_speed;  break;
            }
            velocity->SetVelocity(initialVelocity);
        }

        m_timerStarted = true;
    }

    float elapsed = m_lifespanTimer.Elapsed();

    // Trigger cool effect if the boulder is about to fade
    if (!m_fadingEffectTriggered && elapsed >= m_lifespan - 1.0f) { // Trigger 1 second before fade
        CoolEffect();
        m_fadingEffectTriggered = true;
    }

    // Check if the lifespan has expired
    if (elapsed >= m_lifespan) {
        wolf::Log("BoulderTrapComponent: Lifespan expired, deleting boulder.");
        GetGameObject()->Delete();
    }
}

void BoulderTrapComponent::CoolEffect() {
    auto* transform = GetGameObject()->GetComponent<wolf::Transform2D>();
    if (!transform) return;

    // Gradually shrink scale
    glm::vec2 currentScale = transform->GetGlobalScale();
    glm::vec2 newScale = currentScale * 0.95f; // Reduce scale slightly per call
    transform->SetScale(newScale);

    wolf::Log("BoulderTrapComponent: Cool effect in progress. Scale reduced.");
}
