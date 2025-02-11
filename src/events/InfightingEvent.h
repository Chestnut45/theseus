#pragma once
#include <W_GameObject.h>

struct InfightingEvent{
    wolf::GameObject* m_pAttacker;  // The enemy that attacked
    wolf::GameObject* m_pVictim;    // The enemy that got hit

    InfightingEvent(wolf::GameObject* attacker, wolf::GameObject* victim)
        : m_pAttacker(attacker), m_pVictim(victim) {}
};
