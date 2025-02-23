//-----------------------------------------------------------------------------
// File: HarpyBuilder.h
// Original Author:	???
// Modifications: Nguyễn Minh Nhật
// Constructs harpy enemies.
//-----------------------------------------------------------------------------
#pragma once

#include <wolf.h>
#include "HarpyController.h"
#include "ColliderManager.h"
#include <EnemyDataLoader.h>

class HarpyBuilder
{
public:
    explicit HarpyBuilder(wolf::Scene& scene) : m_scene(scene) {}
    wolf::GameObject& BuildHarpy(const EnemyData& data, const glm::vec2& position);

private:
    // Reference to the scene where the Harpy will be created
    wolf::Scene& m_scene;

    // Pointer to the created Harpy GameObject
    wolf::GameObject* m_pHarbyoBJECT = nullptr;

    // Pointer to the HarpyController to initialize the Harpy behavior
    HarpyController* m_pHarpyController = nullptr;
};
