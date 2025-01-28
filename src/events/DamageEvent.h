#pragma once

#include <W_GameObject.h>

struct DamageEvent
{
    bool m_pierce = false;
    float m_damage = 0.0f;
    wolf::GameObject* m_pDamagedObject = nullptr;
};