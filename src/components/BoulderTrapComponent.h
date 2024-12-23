#pragma once
#include "W_BaseComponent.h"
#include "ColliderManager.h"
#include "W_Timer.h"

enum class BoulderDirection {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class BoulderTrapComponent : public wolf::BaseComponent {
public:
    BoulderTrapComponent(ColliderManager* colliderManager, BoulderDirection direction, float speed, float lifespan);
    void Update(float delta);

private:
    void CoolEffect();

    ColliderManager* m_colliderManager = nullptr;
    BoulderDirection m_direction;
    float m_speed;
    float m_lifespan;
    bool m_timerStarted = false;
    wolf::Timer m_lifespanTimer;
    bool m_fadingEffectTriggered = false;
};
