#pragma once

//-----------------------------------------------------------------------------
// File:			W_Scene.h
// Original Author:	D'Anyil Landry
//
// A class representing a hierarchical collection of game objects with arbitrary
// structs or classes as components.
//-----------------------------------------------------------------------------

#include <vector>
#include <utility>
#include <entt/entity/registry.hpp>

#include "W_Camera2D.h"

namespace wolf
{

// Forward declaration
class GameObject;

// Type representing a game object's ID within its scene.
typedef uint32_t GameObjectID;

class Scene
{
// Public interface
public:

    // Create an empty scene with no objects
    Scene();
    ~Scene();

    // Default copy constructor/assignment
    Scene(const Scene&) = default;
    Scene& operator=(const Scene&) = default;

    // Default move constructor/assignment
    Scene(Scene&& other) = default;
    Scene& operator=(Scene&& other) = default;

    // Game object management

    // Create and return a reference to an empty game object.
    GameObject& CreateObject();

    // Create and return a reference to a game object with a Transform2D component
    GameObject& CreateObject2D();

    // Gets a pointer to the game object with the given ID,
    // or a null pointer if no object with that ID exists.
    GameObject* GetObject(GameObjectID id);

    // Deletes a game object and all of its components
    void DeleteObject(GameObjectID id);

    // Deletes all game objects and components
    void Clear();

    // Active camera management
    void SetActiveCamera(Camera2D& camera);
    void RemoveActiveCamera();
    Camera2D* GetActiveCamera() const { return m_pActiveCamera; }

    // Simulation / rendering

    // Updates the scene
    void Update(float delta);

    // Renders the scene using the currently active Camera2D
    void Render(float delta);

    // Helper function to iterate all components of any type(s) efficiently.
    // Returns an iterable container you can use in an auto for loop with structured binding.
    // GameObject is also a valid type to iterate.
    // Example usage is in the README.md
    template <typename... T>
    constexpr auto Each()
    {
        return m_registry.view<T...>().each();
    }

    // Setter/Getter for the player's game object ID
    void SetPlayerID(GameObjectID p_uiGOId);
    GameObjectID const GetPlayerID();

// Data / implementation
protected:

    // Registry that contains all game object and component data
    entt::basic_registry<GameObjectID> m_registry;

    Camera2D* m_pActiveCamera = nullptr;

    GameObjectID m_uiPlayerGOId;

    // Needed for game objects to have access to the scene's registry
    friend class GameObject;
};

// Tests the features and expected behaviour of the scene system
void _SceneTests();

}