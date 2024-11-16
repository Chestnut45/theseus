#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_EventManager.h"
#include "W_GameObject.h"
#include <events/TriggerEvent.h>
#include <events/TrapDestroyedEvent.h>

enum class BoulderDirection {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class BoulderTrapComponent : public wolf::BaseComponent {
public:
    BoulderTrapComponent(ColliderManager* colliderManager, BoulderDirection direction, float speed, float damage);
    ~BoulderTrapComponent();

    void Update(float delta);

private:
    void OnTriggerEvent(const TriggerEvent& event);
    void MoveBoulder(float delta);
    bool CheckCollision(float delta);
    bool CheckForPlayerCollision(float delta);
    bool CheckForEnemyCollision(float delta);
    bool CheckForWallCollision();

    ColliderManager* m_colliderManager = nullptr;
    BoulderDirection m_direction;
    float m_speed;
    float m_damage;
    bool m_activated = false;
};
