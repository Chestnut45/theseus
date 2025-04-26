#pragma once

//-----------------------------------------------------------------------------
// File: SnakeBuilder.h
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật, D'Anyil Landry
// Constructs snake enemies.
//-----------------------------------------------------------------------------

#include <wolf.h>
#include "SnakeController.h"
#include "ColliderManager.h"
#include <EnemyDataLoader.h>

class SnakeBuilder
{
public:
    explicit SnakeBuilder(wolf::Scene& scene) : m_scene(scene) {}
    wolf::GameObject& BuildSnake(const EnemyData& data, const glm::vec2& position);

private:

    wolf::Scene& m_scene;
    wolf::GameObject* m_pSnakeObject = nullptr;
    SnakeController* m_pSnakeController = nullptr;
};
