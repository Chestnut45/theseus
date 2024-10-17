#pragma once

#include <wolf.h>
#include "EnemyController.h"
#include "VelocityComponent.h"
#include "HealthComponent.h"
#include "ColliderComponent.h"
#include "ArmourComponent.h"

// EnemyBuilder class to encapsulate the creation and initialization of an enemy object
class EnemyBuilder
{
public:
    // Constructor takes a reference to the scene
    EnemyBuilder(wolf::Scene& scene)
        : m_scene(scene), m_pEnemyObject(nullptr), m_pEnemyController(nullptr) {}

    // Build the enemy GameObject and return a reference to it
     wolf::GameObject& BuildEnemy(ColliderManager* pColliderManager);

    // Function to get the EnemyController pointer

private:
    wolf::Scene& m_scene;                   // Reference to the scene
    wolf::GameObject* m_pEnemyObject;        // Pointer to the created enemy GameObject
    EnemyController* m_pEnemyController;     // Pointer to the added EnemyController component
};