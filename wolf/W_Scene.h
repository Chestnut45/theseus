#pragma once

//-----------------------------------------------------------------------------
// File:			W_Scene.h
// Original Author:	D'Anyil Landry
//
// A class representing a hierarchical collection of objects with arbitrary
// structs or classes as components.
// 
// Scenes don't directly simulate or render components, but offer efficient
// APIs for iterating individual components or objects containing the same
// group of components.
//-----------------------------------------------------------------------------

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

namespace wolf
{

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

    // Types
    typedef uint32_t ObjectID;
    class Object
    {
    // Public interface
    public:

        // TESTING
        ObjectID GetID() { return (ObjectID)m_handle.entity(); };

        // Used internally but must be public
        // NOTE: Don't instantiate scene objects directly! Use Scene::CreateObject().
        Object(Scene& scene);
        ~Object();

        // Delete copy constructor/assignment
        Object(const Object&) = delete;
        Object& operator=(const Object&) = delete;

        // Delete move constructor/assignment
        Object(Object&& other) = delete;
        Object& operator=(Object&& other) = delete;
    
    // Data / implementation
    private:

        entt::handle m_handle;
    };

    // Object management

    // Returns a reference to a newly-created empty scene object.
    // If no name is supplied, one is automatically generated.
    // If an object with the given name already exists, it is returned instead.
    Object& CreateObject(const std::string& name = "_autogen");

    // Gets a pointer to the object with the given name,
    // or a null pointer if no object with that name exists.
    Object* GetObject(const std::string& name);

    // Deletes all objects and components from the scene
    void Clear();


    // Alternative object management

    // Gets a reference to a newly-created empty scene object.
    Object& CreateObject();

    // Gets a pointer to the object with the given ID,
    // or a null pointer if no object with that ID exists.
    Object* GetObject(ObjectID id);

// Data / implementation
private:

    entt::registry m_registry;

};

}