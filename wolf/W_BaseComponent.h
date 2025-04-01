#pragma once

//-----------------------------------------------------------------------------
// File:			W_BaseComponent.h
// Original Author:	D'Anyil Landry
//
// A class representing a component that has direct access to the game object
// it is attached to.
// 
// NOTE: m_pGameObject is set immediately *following* the constructor of
// a component when created using wolf::GameObject::AddComponent<T>(...)
//-----------------------------------------------------------------------------

namespace wolf
{

// Forward declaration
class GameObject;

class BaseComponent
{

// Public interface
public:

    // Gets a pointer to the game object this component is attached to
    // Returns nullptr if the component is not attached to a game object.
    GameObject* GetGameObject() const { return m_pGameObject; }

// Implementation
private:

    GameObject* m_pGameObject = nullptr;
    
    friend class GameObject;
};

}