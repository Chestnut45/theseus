#pragma once

//-----------------------------------------------------------------------------
// File:			LabyrinthTiles.h
// Original Author:	D'Anyil Landry
//
// A header declaring an enum to name each Tile ID in the labyrinth.
//-----------------------------------------------------------------------------

// Labyrinth tile IDs
// NOTE: Using a normal enum scoped in a struct allows us
// to use the Tile IDs as integers without casting them.
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
        WallTopLeft,
        WallTopRight,
        WallTop
    };
};