#pragma once

//-----------------------------------------------------------------------------
// File:			W_Scene2D.h
// Original Author:	D'Anyil Landry
//
// A class representing a specialized scene with built-in simulation and
// rendering of various 2D components
// 
// Objects created by a Scene2D will automatically have a Transform2D component
//-----------------------------------------------------------------------------

#include "W_Scene.h"
#include "W_Camera2D.h"

namespace wolf
{

class Scene2D : public Scene
{

// Public interface
public:

    Scene2D();
    ~Scene2D();

    // Default copy constructor/assignment
    Scene2D(const Scene2D&) = default;
    Scene2D& operator=(const Scene2D&) = default;

    // Default move constructor/assignment
    Scene2D(Scene2D&& other) = default;
    Scene2D& operator=(Scene2D&& other) = default;

    // Create and return a reference to a game object with a 2D transformation component
    GameObject& CreateObject() override;

    void SetActiveCamera(Camera2D& camera);
    void RemoveActiveCamera();
    Camera2D* GetActiveCamera() const { return m_pActiveCamera; }

    // Updates the scene
    void Update(float delta);

    // Renders the scene using the currently active camera
    void Render();

// Implementation
private:

    Camera2D* m_pActiveCamera = nullptr;
};

}