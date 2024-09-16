#pragma once

#include <wolf.h>
#include "GameStateManager.h"

// Scoped tile enum (does not require casting)
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

class Theseus : public wolf::App
{
    // Interface
    public:

        Theseus();
        ~Theseus();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;
    
    private:

        // The main game scene
        wolf::Scene m_scene;

        // A pointer to the player game object
        wolf::GameObject* m_pPlayerObject = nullptr;

        // Manager for handling game states
        GameStateManager* m_pStateManager = nullptr;
};