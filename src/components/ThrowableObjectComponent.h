#pragma once

#include "ColliderComponent.h"
#include "wolf.h"
#include <glm/glm.hpp>

class ColliderManager;

enum class ThrowableState {
    IDLE,       // Waiting to be picked up
    PICKED_UP,  // Held by player
    THROWN      // Thrown and in motion
};
class ThrowableObjectComponent : public wolf::BaseComponent {
public:
    // Constructor
    ThrowableObjectComponent(float damage, ColliderManager* colliderManager);

    // Update function called every frame
    void Update(float delta);

    // Pick up the object
    void PickUp();

    // Drop the object
    void Drop();

    // Checks if player is close enough to show pickup prompt
    bool IsCloseToPlayer(float distanceThreshold) const;

    // Set the state of the throwable object
    void SetState(ThrowableState newState);

    void SetThrown();

private:
    void FollowPlayer(); // Makes the object follow the player when picked up
    void RenderPickupPrompt();

    // Components and references
    ColliderComponent* m_pCollider = nullptr;
    wolf::Transform2D* m_pTransform = nullptr;
    ColliderManager* m_pColliderManager = nullptr;

    // Physics and state properties
    ThrowableState m_state;
    float m_damage;

    float m_hoverAnimationOffset = 0.0f; // Offset for floating prompt animation
};
