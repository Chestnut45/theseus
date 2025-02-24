//-----------------------------------------------------------------------------
// File: GorgonBuilder.h
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Constructs gorgon enemies.
//-----------------------------------------------------------------------------
#pragma once

#include <wolf.h>
#include "GorgonController.h"
#include "ColliderManager.h"
#include <EnemyDataLoader.h>

class GorgonBuilder
{
public:
    explicit GorgonBuilder(wolf::Scene& scene) : m_scene(scene) {}
    wolf::GameObject& BuildGorgon(const EnemyData& data, const glm::vec2& position);

private:
    // Reference to the scene where the Gorgon will be created
    wolf::Scene& m_scene;

    // Pointer to the created Gorgon GameObject
    wolf::GameObject* m_pGorgonObject = nullptr;

    // Pointer to the GorgonController to initialize the Gorgon behavior
    GorgonController* m_pGorgonController = nullptr;
};
