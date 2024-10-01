#pragma once

#include <wolf.h>
#include "PlayerController.h"

// Forward declarations
class PlayerController;

class PlayerBuilder {
public:
    // Constructor accepting a scene reference
    explicit PlayerBuilder(wolf::Scene& scene);

    // Function to create the player GameObject and add required components
    wolf::GameObject& BuildPlayer();

    // Function to get the created PlayerController
    PlayerController* GetPlayerController() const;

private:
    // Reference to the scene to create objects in
    wolf::Scene& m_scene;

    // Pointer to the player GameObject
    wolf::GameObject* m_pPlayerObject = nullptr;

    // Pointer to the PlayerController component
    PlayerController* m_pPlayerController = nullptr;

};