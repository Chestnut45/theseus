#pragma once

//-----------------------------------------------------------------------------
// File:			ThrwoableObjectComponent.h
// Original Author:	Youssef Ashraf
// ver 1.1
// class responsible for throwable objects
//-----------------------------------------------------------------------------


#include "ColliderComponent.h"
#include "MinitaurController.h"
#include "HealthComponent.h"
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

    //Set Thrown
    void SetThrown();



private:
    void FollowPlayer(); // Makes the object follow the player when picked up
    void RenderPickupPrompt();
    void HandleCollision();
    void CheckLifetime(float delta);
    void HandleEnemyCollision(wolf::GameObject* enemyObject, float damage);
    float m_lifetime = 4.0f; // Timer for self-destruction
    bool m_hasCollided = false; // Flag to check if collision has already occurred

    // Components and references
    ColliderComponent* m_pCollider = nullptr;
    wolf::Transform2D* m_pTransform = nullptr;
    ColliderManager* m_pColliderManager = nullptr;

    // Physics and state properties
    ThrowableState m_state;
    float m_damage;

    float m_hoverAnimationOffset = 0.0f; // Offset for floating prompt animation

    wolf::GameObjectID m_uiPlayerGOId;


};
