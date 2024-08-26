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
    class Object
    {
    // Public interface
    public:

        // 

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


    };

    // Object management

    // Returns a reference to a newly-created empty scene object
    Object& CreateObject(const std::string& name);
    

// Data / implementation
private:

    entt::registry m_registry;

};

}