#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_Timer.h"

enum class BoulderDirection {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class BoulderTrapComponent : public wolf::BaseComponent {
public:
    BoulderTrapComponent(ColliderManager* colliderManager, BoulderDirection direction, float speed, float damage, float lifespan);
    void Update(float delta);

private:
    void MoveBoulder(float delta);

    ColliderManager* m_colliderManager = nullptr;
    BoulderDirection m_direction;
    float m_speed;
    float m_damage;
    float m_lifespan;
    wolf::Timer m_lifespanTimer;
    bool m_timerStarted = false; 
};
