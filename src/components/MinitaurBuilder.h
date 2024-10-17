#pragma once

#include <wolf.h>
#include "MinitaurController.h"
#include "ColliderManager.h"

class MinitaurBuilder
{
public:
    // Constructor to initialize the builder with a scene reference
    explicit MinitaurBuilder(wolf::Scene& scene)
        : m_scene(scene) {}

    // Build the Minitaur and return the created GameObject
    wolf::GameObject& BuildMinitaur(ColliderManager* pColliderManager);

private:
    // Reference to the scene where the Minitaur will be created
    wolf::Scene& m_scene;

    // Pointer to the created Minitaur GameObject
    wolf::GameObject* m_pMinitaurObject = nullptr;

    // Pointer to the MinitaurController to initialize the Minitaur's behavior
    MinitaurController* m_pMinitaurController = nullptr;
};
