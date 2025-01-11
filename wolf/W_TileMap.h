#pragma once

//-----------------------------------------------------------------------------
// File:			W_TileMap.h
// Original Author:	D'Anyil Landry
//
// A renderable component representing a regular 2D grid of textured tiles.
//-----------------------------------------------------------------------------

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include <W_GameObject.h>
#include <W_Grid2D.h>
#include <W_ProgramManager.h>
#include <W_VertexBuffer.h>
#include <W_VertexDeclaration.h>

namespace wolf
{

class TileMap : public wolf::BaseComponent
{

// Interface
public:

    // Empty tile ID
    static const inline int EMPTY_TILE = -1;

    // Create an empty tilemap from (0, 0) to (width - 1, height - 1).
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
    // NOTE: All tile images in a tile set must have the same dimensions.
    // NOTE: The maximum number of tiles in a tile set is 2048.
    // NOTE: Returns false if any files fail to load or the above rules are broken.
    bool LoadTileSet(const std::string& filepath);

    // Gets the tile ID at the given location.
    // NOTE: Returns -1 if tile at position is empty or position is out of bounds.
    int GetTile(int x, int y) const;

    // Sets the tile at the given location to the given ID.
    // NOTE: Logs an error if position is out of bounds.
    // NOTE: Does not validate tileID
    void SetTile(int x, int y, int tileID);

    // Sets every tile in the map to tile
    // Default value is -1
    void Clear(int tile = EMPTY_TILE);

    // Resizes the map to the given dimensions.
    // NOTE: Resizing will clear the map too!
    void Resize(int width, int height, int clearTile = EMPTY_TILE);
    
    // Visibility access / control
    inline bool IsVisible() const { return m_visible; }
    inline void SetVisibility(bool visible) { m_visible = visible; }

    // TODO: Set origin to center of tilemap (including tile texture size)

    // Draw the tilemap at the given position, rotation, and scale in world space
    // Multiplies final pixel color by provided tint color
    // NOTE: Requires a Camera2D to be bound to slot 0 before drawing.
    void Draw(const glm::vec2& position, float rotationRadians = 0.0f, const glm::vec2& scale = glm::vec2(1.0f), const glm::vec3& tint = glm::vec3(1.0f));

    // TODO: Generate a collider component that lines up with collidable tiles
    void GenerateCollider();

// Implementation
private:

    // Grid of tile data
    wolf::Grid2D<int> m_tileGrid;

    // Tile dimensions in pixels
    int m_tileWidth = 0;
    int m_tileHeight = 0;

    // Number of tiles to draw
    int m_tilesToDraw = 0;

    // Tile set texture data
    GLuint m_arrayTexture = 0;
    std::string m_tileSetPath;

    // Per-tilemap GPU resources
    GLuint m_VBO = 0;
    GLuint m_VAO = 0;

    // Flags
    bool m_VBODirty = true;
    bool m_visible = true;

    // Helper methods
    void _UpdateVBO();
    void _GenerateVAO();

    // Static resources

    // Type defining an entry in the tile set ID map
    struct TileSetEntry
    {
        GLuint m_texID = 0;
        GLuint m_refCount = 1;
        glm::ivec2 m_tileSize{0};
    };

    // Map from file path to reference counted tile set array texture ID
    static inline std::unordered_map<std::string, TileSetEntry> s_tileSetIDMap;

    // Static rendering resources
    static inline wolf::Program* s_pProgram = nullptr;
    static inline wolf::VertexBuffer* s_pQuadVertexBuffer = nullptr;
    static inline wolf::IndexBuffer* s_pQuadIndexBuffer = nullptr;

    // Reference counting for static resources
    static inline size_t s_refCount = 0;
};

}