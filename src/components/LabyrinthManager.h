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
//-----------------------------------------------------------------------------

#include <cstdint>
#include <unordered_map>

// Needed for std::hash implementation for glm vector types
#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/hash.hpp>

#include <W_GameObject.h>
#include <W_Grid2D.h>
#include <W_RNG.h>
#include <W_Shapes.h>

#include <LabyrinthTiles.h>

#include <ColliderManager.h>

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

    // Loads a labyrinth config from a YAML file
    void LoadConfig(const std::string& filepath);

    // Saves the current config to a YAML file
    void SaveConfig(const std::string& filepath);

    // Gets the spawn position of the labyrinth in world space
    // NOTE: Returns (0, 0) if the labyrinth is not yet generated
    glm::vec2 GetSpawnLocation() const;

    // Gets the chunk ID for the chunk containing a given world space position
    glm::ivec2 GetChunkID(const glm::vec2& worldPosition) const;

    // Gets a pointer to the chunk object with the given ID
    // NOTE: Returns nullptr if no chunk exists with the given ID
    wolf::GameObject* GetChunk(const glm::ivec2& chunkID) const;

    // Deletes the chunk at the given ID if it exists
    void DeleteChunk(const glm::ivec2& chunkID);

    // Converts a world space position to tile coordinates
    // NOTE: Returns (-1, -1) if the position is not on a valid tile
    glm::ivec2 GetTilePosition(const glm::vec2& worldPosition) const;

    // Gets the tile ID at the given tile position of the labyrinth
    // NOTE: Returns -1 if the tile is empty
    // NOTE: Returns -2 if out of bounds or not yet generated
    int GetTile(int x, int y) const;

    // Sets the tile at the given tile position of the labyrinth
    // NOTE: Does nothing if out of bounds
    // NOTE: Does not validate tileID
    void SetTile(int x, int y, int tileID);

    // Resets all properties to their defaults
    void Reset();

    // Helper methods

    // Gets a pointer to the first player object in the scene
    // NOTE: Returns nullptr if no player is found
    wolf::GameObject* GetPlayer() const;

    // Constants
    static const inline int MIN_LABYRINTH_DIM = 5;
    static const inline int MAX_LABYRINTH_DIM = 16'383;
    static const inline int TILE_SIZE = 32;
    static const inline int CHUNK_SIZE = 16;
    static const inline int SCALE = 3;

// Implementation
private:

    // Pseudo random number generator
    wolf::RNG m_rng;

    // Labyrinth dimensions (in tiles)
    int m_width = 125;
    int m_height = 125;

    // Spawn area settings
    glm::ivec2 m_spawnPatchSize = glm::ivec2(25);
    glm::ivec2 m_spawnRoomSize = glm::ivec2(5);

    // Flags
    bool m_randomizeSeed = false;
    bool m_isGenerated = false;

    // Tile data

    // Logical tile types (not including visual variations)
    enum class LogicalTile
    {
        Unvisited,
        Door,
        Floor,
        OccupiedFloor,
        Grass,
        Wall,
    };

    // Grid of logical tiles
    wolf::Grid2D<LogicalTile> m_labyrinthGrid{m_width, m_height, LogicalTile::Unvisited};

    // Room data

    // Definition of a room to be generated into the labyrinth
    struct Room
    {
        // Identifier (non-unique)
        std::string m_name{"New Room"};

        // Bounds of the room in labyrinth space (measured in tiles)
        wolf::IRectangle m_bounds{1, 2, 2, 1};

        // The number of instances of this room to generate
        // NOTE: Instances are generated with different rng
        // values so that they won't be identical copies.
        int m_instances = 1;

        // Whether to force the room's placement or not
        // NOTE: Each instance will get MAX_PLACEMENT_ATTEMPTS
        // attempts before giving up and failing placement. If
        // placement fails and m_force is true, the room will be placed
        // anyway (possibly overlapping with another generated room)
        bool m_force = false;

        // Number of attempts each room gets to be placed
        static const inline int MAX_PLACEMENT_ATTEMPTS = 128;

        // Position types
        enum class PositionType
        {
            Manual,
            Random,
            RandomRadius,
        };
        static const inline char* s_positionTypeNames[] = {"Manual", "Random", "Random Radius"};

        // Position data
        // Measured as the bottom-left floor tile of the room
        PositionType m_positionType = PositionType::Random;
        glm::ivec2 m_randomRadiusPosition{1, 1};
        int m_randomRadius = 16;

        // Size types
        enum class SizeType
        {
            Manual,
            RandomMinMax,
        };
        static const inline char* s_sizeTypeNames[] = {"Manual", "Random Min Max"};

        // Size data
        // Measured in usable floor tiles
        SizeType m_sizeType = SizeType::RandomMinMax;
        glm::ivec2 m_minSize{3, 3};
        glm::ivec2 m_maxSize{9, 9};

        // Entity types
        enum class EntityType
        {
            Minitaur,
            CommonChest,
            UncommonChest,
            RareChest,
            EpicChest,
            LegendaryChest,
            DaedalusDispensary, // !-- Aurora added this --!
        };
        static const inline char* s_entityTypeNames[] = {"Minitaur", "Common Chest", "Uncommon Chest", "Rare Chest", "Epic Chest", "Legendary Chest", "Daedalus Dispensary"};

        // Entity spawn data structure
        struct EntitySpawnData
        {
            EntityType m_type = EntityType::Minitaur;
            int m_amount = 1;
        };

        // Entity spawn data
        std::vector<EntitySpawnData> m_entitySpawns;
    };

    // List of all rooms to be generated in the labyrinth
    std::vector<Room> m_rooms;

    // Non-owning pointer to collider manager. Necessary for building minitaurs
    ColliderManager* m_pColliderManager = nullptr;
    friend class PlayState;

    // Map of tile positions to section numbers
    std::unordered_map<glm::ivec2, int> m_tileSectionMap;

    // Data structure for a connector
    struct Connector
    {
        glm::ivec2 m_pos;
        int m_connection;
    };

    // Data structure for a section
    struct Section
    {
        // Map of connected sections
        std::unordered_map<int, bool> m_connected;

        // List of connectors to other sections
        std::vector<Connector> m_connectors;
    };

    // List of sections
    std::vector<Section> m_sections;

    // Chunk management

    struct ChunkData
    {
        wolf::GameObject* m_pObject = nullptr;
        bool active = false;
    };

    // Map of chunk IDs to chunk game object pointers
    std::unordered_map<glm::ivec2, ChunkData> m_chunkMap;

    // Queues
    std::vector<glm::ivec2> m_chunkActivateQueue;
    std::vector<glm::ivec2> m_chunkDeactivateQueue;

    // Cached ID of chunk player was in last frame
    glm::ivec2 m_prevChunk = glm::ivec2(0);

    // Helper methods

    // Activates a chunk, recursively updating all child objects' flags.
    void ActivateChunk(const glm::ivec2& chunkID);

    // Deactivates a chunk, recursively updating all child objects' flags.
    void DeactivateChunk(const glm::ivec2& chunkID);

    // Attempts to place all rooms and returns a vector of those successfully placed
    std::vector<Room> PlaceRooms();

    // Carves the maze into the labyrinth using the current settings
    void CarveMaze();

    // Guarantees connectivity between all rooms and the entrance of the maze
    void ConnectRooms(const std::vector<Room>& placedRooms);

    // Generates all chunk objects into the scene for the current maze
    void GenerateChunks();

    // Spawns all the entities from placed rooms into the chunks
    void PopulateEntities(const std::vector<Room>& placedRooms);

    // Generates the entrance room to the maze
    void GenerateEntrance();
};