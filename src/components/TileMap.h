#pragma once

//-----------------------------------------------------------------------------
// File:			TileMap.h
// Original Author:	D'Anyil Landry
//
// A renderable component representing a regular 2D grid of textured tiles.
//-----------------------------------------------------------------------------

#include <string>
#include <vector>
#include <unordered_map>

#include <W_GameObject.h>
#include <W_ProgramManager.h>
#include <W_VertexBuffer.h>
#include <W_VertexDeclaration.h>

class TileMap : public wolf::BaseComponent
{

// Interface
public:

    // Create an empty tilemap from (0, 0) to (width, height).
    TileMap(int width, int height);
    ~TileMap();

    // Delete copy constructor/assignment
    TileMap(const TileMap&) = delete;
    TileMap& operator=(const TileMap&) = delete;

    // Delete move constructor/assignment
    TileMap(TileMap&& other) = delete;
    TileMap& operator=(TileMap&& other) = delete;

    // Loads a tile set file from disk.
    // 
    // A tile set file is a simple text file listing the paths
    // to all tile images in the set, one per line.
    // 
    // IDs will be assigned to the tiles in line order, starting from 0.
    // 
    // NOTE: All tile textures must have the same dimensions.
    // NOTE: The maximum number of tiles in a tile set is 2048.
    // NOTE: Returns false if any files fail to load or the above rules are broken.
    bool LoadTileSet(const std::string& filepath);

    // Gets the tile ID at the given location.
    // NOTE: Returns -1 if position is empty or out of bounds.
    int GetTile(int x, int y) const;

    // Sets the tile at the given location to the given ID.
    // NOTE: Does nothing if position is out of bounds.
    // NOTE: Does nothing if tileID is invalid.
    void SetTile(int x, int y, int tileID);

    // Deletes all tiles in the map.
    void Clear();

    // Resizes the map to the given dimensions.
    // NOTE: Resizing will clear the map too!
    void Resize(int width, int height);

    // TODO: Rendering interface

// Implementation
private:

    // Dimensions
    int m_width;
    int m_height;

    // Flattened tile array
    std::vector<int> m_tileData;

    // Array texture object ID
    GLuint m_arrayTexture = 0;

    // File path to currently loaded tile set
    std::string m_tileSetPath;

    // Type defining an entry in the tile set ID map
    struct TileSetEntry
    {
        GLuint m_texID = 0;
        GLuint m_refCount = 1;
    };

    // Static map of loaded tile sets
    static inline std::unordered_map<std::string, TileSetEntry> s_tileSetIDMap;

    // Static rendering resources
    static inline wolf::Program* s_pProgram = nullptr;
    static inline wolf::VertexBuffer* s_pVertexBuffer = nullptr;
    static inline wolf::IndexBuffer* s_pIndexBuffer = nullptr;
    static inline wolf::VertexDeclaration* s_pVAO = nullptr;
};