#pragma once

//-----------------------------------------------------------------------------
// File:			W_GameObject.h
// Original Author:	D'Anyil Landry
//
// A class representing a game object that belongs to a scene. Game objects
// can have components of any type attached to them, and can form hierarchical
// relationships with other game objects within the same scene.
//-----------------------------------------------------------------------------

#include <cstdint>

#include "W_Scene.h"
#include "W_BaseComponent.h"

namespace wolf
{

class GameObject
{
// Public interface
public:

    ~GameObject();

    // Delete copy constructor/assignment
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    // Delete move constructor/assignment
    GameObject(GameObject&& other) = delete;
    GameObject& operator=(GameObject&& other) = delete;

    // General management

    // Retrieve the ID of this game object.
    inline GameObjectID GetID() { return m_id; }

    // Retrieve a reference to the scene this game object belongs to
    inline Scene& GetScene() { return m_scene; }

    // Deletes this game object and all of its components from the scene
    inline void Delete() { m_scene.DeleteObject(m_id); }

    // Component management

    // Adds a component to the game object by in-place construction.
    // Returns a non-const reference to the newly-created component.
    // NOTE: Pass your component's constructor arguments directly to this function!
    template <typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        T& component = m_scene.m_registry.emplace<T>(m_id, args...);

        // Compile-time check for base component derived components
        if constexpr (std::is_base_of_v<BaseComponent, T>)
        {
            component.m_pGameObject = this;
        }

        return component;
    }

    // Gets a pointer to the component, or nullptr if it doesn't exist
    template <typename T>
    T* GetComponent()
    {
        return m_scene.m_registry.try_get<T>(m_id);
    }

    // Deletes the component, if it exists
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

    // Returns true if the object or any of its children has any of the given components
    template <typename... T>
    bool HasAnyRecursive()
    {
        std::function<bool(GameObject*)> HasAnyRecurse = [&, this](GameObject* pGO) -> bool {
            bool has = m_scene.m_registry.any_of<T...>(pGO->GetID());
            if (has) return true;
            for (auto child : pGO->GetChildren())
            {
                bool childHas = HasAnyRecurse(child);
                if (childHas) return true;
            }
            return false;
        };
        return HasAnyRecurse(this);
    }

    // Returns the first instance in the game object hierarchy of the given component
    // Returns nullptr if none could be found
    template <typename T>
    T* FindChildComponent()
    {
        std::function<T*(GameObject*)> FindRecurse = [&, this](GameObject* pGO) -> T* {
            T* comp = m_scene.m_registry.try_get<T>(pGO->GetID());
            if (comp) return comp;
            for (auto child : pGO->GetChildren())
            {
                T* childComp = FindRecurse(child);
                if (childComp) return childComp;
            }
            return nullptr;
        };
        return FindRecurse(this);
    }

    // Hierarchy management
    
    // Adds a game object to our list of children
    // If the object already has a parent, it is removed from that parent first
    void AddChild(GameObject& object);

    // Removes the given child game object from our list of children, and
    // updates the child object's parent pointer to be empty
    void RemoveChild(GameObject& object);

    // Removes and deletes all child game objects
    void DeleteAllChildren();

    // Returns a pointer to the parent game object, or nullptr if we have none
    inline GameObject* GetParent() const { return m_parent; }

    // Gets a const reference to the list of child game object pointers
    inline const std::vector<GameObject*>& GetChildren() const { return m_children; }

    // True if this object has 1 or more children
    inline bool HasChildren() const { return m_children.size() > 0; }

    // Used internally but must be public due to allocator rules.
    // NOTE: Don't instantiate Objects directly! Use Scene::CreateObject().
    GameObject(Scene& scene, GameObjectID id);

// Data / implementation
private:

    // Non-owning reference to the scene that created this object
    Scene& m_scene;

    // Our ID within the scene
    const GameObjectID m_id;

    // Pointer to our parent game object, if any
    GameObject* m_parent = nullptr;

    // List of child objects
    std::vector<GameObject*> m_children;

    // This token guarantees pointer stability for objects within the scene.
    // Add it to your component class if you want to store and reuse pointers
    // to the component type over multiple frames.
    static constexpr auto in_place_delete = true;

    // Necessary for the scene to create and delete game objects
    friend class Scene;
};

};