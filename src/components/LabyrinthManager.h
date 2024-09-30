#pragma once

//-----------------------------------------------------------------------------
// File:			LabyrinthManager.h
// Original Author:	D'Anyil Landry
//
// A class representing a game component used to generate and update the labyrinth.
// 
// When attached to a game object, calling Generate() will create all the
// necessary objects and components to represent the labyrinth and add them
// all as child objects of the object the manager is attached to.
// 
// For now, it will only generate a test tilemap, but later it will manage the
// chunk loading system as well as enemy spawns, items, etc.
//-----------------------------------------------------------------------------

#include <cstdint>

#include <W_GameObject.h>
#include <W_RNG.h>
#include <W_Shapes.h>

#include "../LabyrinthTiles.h"

class LabyrinthManager : public wolf::BaseComponent
{

// Public interface
public:

    // Create an empty labyrinth manager component with default settings
    LabyrinthManager();
    ~LabyrinthManager();

    // Delete copy constructor/assignment
    LabyrinthManager(const LabyrinthManager&) = delete;
    LabyrinthManager& operator=(const LabyrinthManager&) = delete;

    // Delete move constructor/assignment
    LabyrinthManager(LabyrinthManager&& other) = delete;
    LabyrinthManager& operator=(LabyrinthManager&& other) = delete;

    // Updates the labyrinth and manages loaded chunks based on the currently active camera
    void Update(float delta);

    // TODO: Accessors and mutators for procedural generation config properties

    // Generates the labyrinth and all of its game objects with the current config
    // NOTE: Adds all game objects as child objects to this component's object
    void GenerateLabyrinth();

    // Deletes the labyrinth and all of its generated game objects
    // NOTE: Deletes all child objects of this component's object
    void DestroyLabyrinth();

    // Destroys the labyrinth and then regenerates it with the current config
    void Regenerate();

    // Display the GUI for editing labyrinth configs and regenerating
    void ShowGUI();

// Implementation
private:

    // Pseudo random number generator
    wolf::RNG m_rng;

    // Labyrinth dimensions (in tiles)
    int m_width = 0;
    int m_height = 0;

    // Flags
    bool m_randomizeSeed = false;

    // Definition of a room to be generated into the labyrinth
    struct Room
    {
        static const int MAX_SIZE = 2048;

        // Default constructor
        Room() {}
        
        // Constructor taking the bounding rectangle
        Room(const wolf::IRectangle& bounds) : m_bounds(bounds) {}

        // Bounds of the room in labyrinth space (origin at bottom left corner)
        wolf::IRectangle m_bounds;
    };

    // List of all rooms in the labyrinth
    std::vector<Room> m_rooms;

    // TODO: Tweakable progression / difficulty parameters (connectivity, spawn rates, etc.)
};