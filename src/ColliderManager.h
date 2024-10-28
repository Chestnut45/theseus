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
#include "components/VelocityComponent.h"

class ColliderComponent;

class ColliderManager
{
friend ColliderComponent;

public:
    ColliderManager(wolf::Scene* p_scene);
    virtual ~ColliderManager();

    void Update(float p_delta);
    bool IsColliding(ColliderComponent* p_colliderComponent1, ColliderComponent* p_colliderComponent2, float p_delta);

private:
    wolf::Scene* m_scene = nullptr;

    void RemoveFlagged();
    void CheckCollisions(float p_delta);    
   
    bool StandardAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2);
    bool StandardAABBBroadphase(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta);
    float SweptAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta);
    bool StandardAABBWithSliding(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta);
    void SlideAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta);
    void PushAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta);

    void addNewVelocity(wolf::GameObjectID p_id, glm::vec2 p_new_velocity);
    void ApplyNewVelocities();

    std::vector<wolf::GameObjectID> m_vToBeDestroyed;
    std::map<wolf::GameObjectID, std::vector<glm::vec2>> m_mNewVelocityVectors;
};
