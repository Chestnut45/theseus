//-----------------------------------------------------------------------------
// File: MinitaurBuilder.h
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Constructs minitaur enemies.
//-----------------------------------------------------------------------------
#pragma once

#include <wolf.h>
#include "MinitaurController.h"
#include "ColliderManager.h"
#include <EnemyDataLoader.h>

class MinitaurBuilder
{
public:
    explicit MinitaurBuilder(wolf::Scene& scene) : m_scene(scene) {}
    wolf::GameObject& BuildMinitaur(const EnemyData& data, const glm::vec2& position);

private:
    // Reference to the scene where the Minitaur will be created
    wolf::Scene& m_scene;

    // Pointer to the created Minitaur GameObject
    wolf::GameObject* m_pMinitaurObject = nullptr;

    // Pointer to the MinitaurController to initialize the Minitaur's behavior
    MinitaurController* m_pMinitaurController = nullptr;
};
