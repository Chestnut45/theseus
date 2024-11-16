#include "BoulderTrapComponent.h"
#include "W_GameObject.h"
#include "W_EventManager.h"

BoulderTrapComponent::BoulderTrapComponent(ColliderManager* colliderManager, BoulderDirection direction, float speed, float damage, float lifespan)
    : m_colliderManager(colliderManager), m_direction(direction), m_speed(speed), m_damage(damage), m_lifespan(lifespan), m_timerStarted(false) {}

void BoulderTrapComponent::Update(float delta) {
    // Start the lifespan timer only when the boulder starts moving
    if (!m_timerStarted) {
        m_lifespanTimer.Start();
        m_timerStarted = true;
    }

    // Check if the lifespan has expired
    if (m_lifespanTimer.Elapsed() >= m_lifespan) {
        wolf::Log("BoulderTrapComponent: Lifespan expired, deleting boulder.");
        GetGameObject()->Delete();
        return;
    }

    // Move the boulder based on its direction
    MoveBoulder(delta);
}

void BoulderTrapComponent::MoveBoulder(float delta) {
    auto* transform = GetGameObject()->GetComponent<wolf::Transform2D>();
    if (!transform) return;

    glm::vec2 movement(0.0f);

    switch (m_direction) {
        case BoulderDirection::UP:
            movement.y -= m_speed * delta;
            break;
        case BoulderDirection::DOWN:
            movement.y += m_speed * delta;
            break;
        case BoulderDirection::LEFT:
            movement.x -= m_speed * delta;
            break;
        case BoulderDirection::RIGHT:
            movement.x += m_speed * delta;
            break;
    }

    transform->Translate(movement);
}
