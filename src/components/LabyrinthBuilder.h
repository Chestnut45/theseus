#pragma once

//-----------------------------------------------------------------------------
// File:			LabyrinthBuilder.h
// Original Author:	D'Anyil Landry
//
// A class representing a game component used to build the labyrinth.
// 
// When attached to a game object, calling Generate() will create all the
// necessary objects and components to represent the labyrinth and add them
// all as child objects of the builder.
// 
// For now, it will only generate a test tilemap, but later it will manage the
// chunk loading system as well as enemy spawns, items, etc.
//-----------------------------------------------------------------------------

#include <cstdint>

#include <W_GameObject.h>
#include <W_RNG.h>

// Labyrinth tile IDs (scoped enum)
struct Tile
{
    typedef int type;
    enum : type
    {
        Empty = -1,
        BorderedGrass = 0,
        Bricks,
        FloorSmallSquares,
        FloorSpiralGold,
        FloorSpiral,
        FloorSquareGold,
        FloorSquare,
        Grass,
        WallBottomLeft,
        WallBottomRight,
        WallBottom,
        WallChest,
        WallHelmet,
        WallLeft,
        WallMaze,
        WallMinotaur,
        WallPillars,
        WallPot,
        WallRight,
        WallSpiral,
        WallSquare,
        WalTopLeft,
        WallTopRight,
        WallTop
    };
};

class LabyrinthBuilder : public wolf::BaseComponent
{

// Public interface
public:

    // Create an empty labyrinth builder with default settings
    LabyrinthBuilder();
    ~LabyrinthBuilder();

    // Delete copy constructor/assignment
    LabyrinthBuilder(const LabyrinthBuilder&) = delete;
    LabyrinthBuilder& operator=(const LabyrinthBuilder&) = delete;

    // Delete move constructor/assignment
    LabyrinthBuilder(LabyrinthBuilder&& other) = delete;
    LabyrinthBuilder& operator=(LabyrinthBuilder&& other) = delete;

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

    // Seed used for the rng during generation
    int m_seed = 0;

    // Pseudo random number generator
    wolf::RNG m_RNG;

    // Labyrinth dimensions (in tiles)
    int m_width = 0;
    int m_height = 0;

    // Flags
    bool m_randomizeSeed = false;

    // TODO: Room data

    // TODO: Tweakable progression / difficulty parameters (connectivity, spawn rates, etc.)
};