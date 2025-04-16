#pragma once

#include <LabyrinthManager.h>

struct LabyrinthRegenerateEvent
{
    LabyrinthManager* m_pLabyrinthManager = nullptr;

    LabyrinthRegenerateEvent(LabyrinthManager* pManager)
        : m_pLabyrinthManager(pManager)
    {
    }
};

struct LabyrinthDestroyEvent
{
    LabyrinthManager* m_pLabyrinthManager = nullptr;

    LabyrinthDestroyEvent(LabyrinthManager* pManager)
        : m_pLabyrinthManager(pManager)
    {
    }
};