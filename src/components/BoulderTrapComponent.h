#pragma once

#include "W_BaseComponent.h"
#include "LabyrinthManager.h"
#include "ColliderManager.h"
#include "TriggerComponent.h"
#include "W_Timer.h"
#include "glm/vec2.hpp"

// Enum for specifying boulder movement direction
enum class BoulderDirection {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class BoulderTrapComponent : public wolf::BaseComponent {
public:
    // Constructor
    BoulderTrapComponent(TriggerComponent* trigger, ColliderManager* colliderManager, BoulderDirection direction, float speed, float lifespan);

    // Update method called every frame
    void Update(float delta);

private:
    // Helper methods
    void InitializeBoulder();                     // Initializes the boulder (timer, velocity, etc.)
    glm::vec2 CalculateInitialVelocity() const;   // Calculates initial velocity based on direction
    void ApplyCoolEffect(float delta, float elapsed); // Handles the cool effects (scaling, glowing, fading)

    // Private members
    TriggerComponent* m_pTrigger = nullptr;
    ColliderManager* m_colliderManager = nullptr; // Reference to the ColliderManager
    BoulderDirection m_direction;                // Direction the boulder will move
    float m_speed;                               // Speed of the boulder
    float m_lifespan;                            // Lifespan of the boulder (in seconds)
    bool m_timerStarted = false;                 // Flag to check if the timer has started
    wolf::Timer m_lifespanTimer;                 // Timer to track the boulder's lifespan
    bool m_fadingEffectTriggered = false;        // Flag to check if the fading effect has been triggered

    int m_damage = 15; // Damage dealt to the player
    float m_knockbackForce = 1500.0f; // Knockback force applied to the player

    bool CheckForEntityCollision(float delta); // Method to check for collision and apply damage/knockback
    bool CheckForWallCollision(float delta);
    // Template function for checking collision with different entity types
    template <typename T>
    bool CheckAndHandleCollision(float delta, ColliderComponent* boulderCollider);

    LabyrinthManager* m_pLabyrinthManager = nullptr; // Store a reference to LabyrinthManager
};
