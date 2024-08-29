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

#include <vector>
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
    
    typedef uint32_t ObjectID;
    
    // An object within a scene hierarchy.
    // Can have components of arbitrary type attached.
    class Object
    {
    // Public interface
    public:

        ~Object();

        // Delete copy constructor/assignment
        Object(const Object&) = delete;
        Object& operator=(const Object&) = delete;

        // Delete move constructor/assignment
        Object(Object&& other) = delete;
        Object& operator=(Object&& other) = delete;

        // General management

        // Retrieve the ID of this object.
        inline ObjectID GetID() { return m_id; }

        // Retrieve a reference to the scene this object belongs to
        inline Scene& GetScene() { return m_scene; }

        // Deletes this object and all its components from the scene
        inline void Delete() { m_scene.DeleteObject(m_id); }

        // Component management

        // Adds a component to the object by in-place construction.
        // Pass your component's constructor arguments directly to this function!
        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            return m_scene.m_registry.emplace<T>(m_id, args...);
        }

        // Gets a pointer to the component, or nullptr if it doesn't exist
        template <typename T>
        T* GetComponent()
        {
            return m_scene.m_registry.try_get<T>(m_id);
        }

        // Deletes the component from the object, if it exists
        template <typename T>
        void DeleteComponent()
        {
            m_scene.m_registry.remove<T>(m_id);
        }

        // Returns true if the object has all of the given components
        template <typename... T>
        bool HasAll()
        {
            return m_scene.m_registry.all_of<T...>(m_id);
        }
        
        // Returns true if the object has any of the given components
        template <typename... T>
        bool HasAny()
        {
            return m_scene.m_registry.any_of<T...>(m_id);
        }

        // Hierarchy management
        
        // Adds an object to our list of children
        // If the object already has a parent, it is removed from that parent first
        void AddChild(Object& object);

        // Removes the given child object from our list of children, and
        // updates the child object's parent pointer to be empty
        void RemoveChild(Object& object);

        // Returns a pointer to the parent object, or nullptr if we have none
        inline Object* GetParent() const { return m_parent; }

        // Gets the list of children objects by id
        inline const std::vector<Object*>& GetChildren() const { return m_children; }

        // True if this object has 1 or more children
        inline bool HasChildren() const { return m_children.size() > 0; }

        // Used internally but must be public
        // NOTE: Don't instantiate Objects directly! Use Scene::CreateObject().
        Object(Scene& scene, ObjectID id);
    
    // Data / implementation
    private:

        // Non-owning reference to the scene that created this object
        Scene& m_scene;

        // Our ID within the scene
        const ObjectID m_id;

        // Pointer to our parent object, if any
        Object* m_parent = nullptr;

        // List of child objects
        std::vector<Object*> m_children;

        // This token guarantees pointer stability for objects within the scene.
        // Add it to your component class if you want to store and reuse pointers
        // to the component type over multiple frames.
        static constexpr auto in_place_delete = true;

        // Necessary for the scene to create and delete objects
        friend class Scene;
    };

    // Object management

    // Create and return a reference to an empty scene object.
    Object& CreateObject();

    // Gets a pointer to the object with the given ID,
    // or a null pointer if no object with that ID exists.
    Object* GetObject(ObjectID id);

    // Deletes an object and all of its components
    void DeleteObject(ObjectID id);

    // Deletes all objects and components
    void Clear();

    // Helper function to iterate all components of any type(s) efficiently.
    // Returns an iterable container you can use in an auto for loop with structured binding.
    // Example usage is in the README.md
    template <typename... T>
    constexpr auto Each()
    {
        return m_registry.view<T&&...>().each();
    }

    // Helper function to iterate all objects in the scene efficiently.
    // Returns an iterable container you can use in an auto for loop with structured binding.
    // Example usage is in the README.md
    auto EachObject()
    {
        return m_registry.view<Object>().each();
    }

// Data / implementation
private:

    // Registry that contains all object and component data
    entt::basic_registry<ObjectID> m_registry;
};

// Tests the features and expected behaviour of the scene system
void _SceneTests();

}