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
#include <unordered_map>

// Needed for std::hash implementation for glm vector types
#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/hash.hpp>

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

    // Constants
    static const inline int MIN_LABYRINTH_DIM = 4;
    static const inline int MAX_LABYRINTH_DIM = 16'384;
    static const inline int LABYRINTH_TILE_SIZE = 32;
    static const inline int CHUNK_SIZE = 64;

// Implementation
private:

    // Pseudo random number generator
    wolf::RNG m_rng;

    // Labyrinth dimensions (in tiles)
    int m_width = 256;
    int m_height = 256;

    // TODO: Tweakable progression / difficulty parameters (connectivity, spawn rates, etc.)

    // Flags
    bool m_randomizeSeed = false;

    // Room data

    // Definition of a room to be generated into the labyrinth
    struct Room
    {
        // Identifier (non-unique)
        std::string m_name{"New Room"};

        // Bounds of the room in labyrinth space (measured in tiles)
        wolf::IRectangle m_bounds{2, 6, 6, 2};

        // TODO: Custom entity spawns
    };

    // List of all rooms in the labyrinth
    std::vector<Room> m_rooms;

    // TODO: Functions to generate rooms of specific types
    // Example: Room GenerateRoom(Room::Type, ...)

    // Chunk management

    // Map of chunk IDs to chunk game object pointers
    std::unordered_map<glm::ivec2, wolf::GameObject*> m_chunkMap;
};