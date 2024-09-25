//-----------------------------------------------------------------------------
// File: ColliderManager.h
// Original Author: Nguyễn Minh Nhật
// Manages collider collisions.
// User Guide:
//     + Create new Manager object before any game object is added to scene
//     + Init() to pass reference to scene
//     + Call Update() every frame
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <ranges>
#include <vector>
#include <wolf.h>

#include "components/HealthComponent.h"
#include "components/ColliderComponent.h"

class ColliderComponent;

class ColliderManager
{
friend ColliderComponent;

public:
    ColliderManager(wolf::Scene* p_scene);
    virtual ~ColliderManager();

    void Update();

private:
    wolf::Scene* m_scene = nullptr;

    bool IsColliding(ColliderComponent* p_colliderComponent1, ColliderComponent* p_colliderComponent2);
    bool IsValidForCollisionCheck(ColliderComponent* p_colliderComponent1, ColliderComponent* p_colliderComponent2);
    void RemoveFlagged();
    void CheckCollisions();

    std::vector<ColliderComponent *> m_vToBeDestroyed;
};