#pragma once

//-----------------------------------------------------------------------------
// File:			TileMap.h
// Original Author:	D'Anyil Landry
//
// A renderable component representing a map of textured tiles.
//-----------------------------------------------------------------------------

#include <W_GameObject.h>

class TileMap : public wolf::BaseComponent
{

// Interface
public:

    // Create an empty tilemap
    TileMap();
    ~TileMap();

    // Delete copy constructor/assignment
    TileMap(const TileMap&) = delete;
    TileMap& operator=(const TileMap&) = delete;

    // Delete move constructor/assignment
    TileMap(TileMap&& other) = delete;
    TileMap& operator=(TileMap&& other) = delete;

    // TODO: Texture / tile loading interface

    // TODO: Procedural generation interface

    // TODO: Rendering interface

// Implementation
private:

    // TODO: Tile representation (flattened 2D array of Tiles)

    // TODO: Rendering architecture (array textures / buffer objects / shaders)
};