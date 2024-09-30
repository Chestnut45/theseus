#include "LabyrinthManager.h"

// ImGui for GUI windows
#include <imgui/imgui.h>

// Icon font definitions
#include <IconsFontAwesome6.h>

#include <W_Logging.h>
#include <W_TileMap.h>
#include <W_Transform2D.h>

LabyrinthManager::LabyrinthManager()
{
}

LabyrinthManager::~LabyrinthManager()
{
}

void LabyrinthManager::Update(float delta)
{
    // TODO: Update active chunks based on active camera
}

void LabyrinthManager::GenerateLabyrinth()
{
    // Only bother if valid generation parameters
    if (m_width <= 0 || m_height <= 0)
    {
        wolf::Error("Can't generate labyrinth, invalid width / height");
        return;
    }

    auto* object = GetGameObject();
    if (!object)
    {
        wolf::Error("Labyrinth generator not attached to a GameObject");
        return;
    }

    // Initialize scale of all labyrinth objects
    auto* pTransform = object->GetComponent<wolf::Transform2D>();
    if (!pTransform) pTransform = &object->AddComponent<wolf::Transform2D>();
    pTransform->SetScale(glm::vec2(3));

    // Grab a scene reference
    auto& scene = object->GetScene();

    // Reseed the rng before generating
    if (m_randomizeSeed) m_rng.SetSeed(m_rng.NextInt(0, INT32_MAX));
    else m_rng.Reseed();

    // Logical tile types (not including visual variations)
    enum class LogicalTile
    {
        Unvisited,
        Door,
        Floor,
        Grass,
        Wall,
    };

    // Initialize global grid of logical tile data for entire labyrinth
    wolf::Grid2D<LogicalTile> labyrinthGrid(m_width, m_height, LogicalTile::Unvisited);

    // Place top/bottom outer walls
    for (int i = 0; i < m_width; ++i)
    {
        labyrinthGrid.Set(i, 0, LogicalTile::Wall);
        labyrinthGrid.Set(i, m_height - 1, LogicalTile::Wall);
    }

    // Place left/right outer walls
    for (int i = 0; i < m_height; ++i)
    {
        labyrinthGrid.Set(0, i, LogicalTile::Wall);
        labyrinthGrid.Set(m_width - 1, i, LogicalTile::Wall);
    }

    // Place all rooms into the labyrinth
    for (int i = 0; i < m_rooms.size(); ++i)
    {
        // Grab references
        const auto& room = m_rooms[i];
        const auto& rect = room.m_bounds;

        for (int y = 0; y < rect.m_size.y; ++y)
        {
            for (int x = 0; x < rect.m_size.x; ++x)
            {
                glm::ivec2 worldPos = {x + rect.m_origin.x, y + rect.m_origin.y};

                // Debug bounds checking
                if (worldPos.x >= m_width || worldPos.y >= m_height)
                {
                    wolf::Warning("Room #", i, ", Tile (", worldPos.x, ", ", worldPos.y, ") out of bounds!");
                    continue;
                }

                // Set border tiles of each room as walls
                if (x == 0 || x == rect.m_size.x - 1 || y == 0 || y == rect.m_size.y - 1)
                {
                    labyrinthGrid.Set(worldPos.x, worldPos.y, LogicalTile::Wall);
                    continue;
                }

                // TODO: Place room-specific tiles / entities
            }
        }
    }

    // TODO: Generate maze paths between all rooms

    // Calculate number of chunks per axis
    const int numChunksX = m_width / CHUNK_SIZE + 1;
    const int numChunksY = m_height / CHUNK_SIZE + 1;
    for (int cy = 0; cy < numChunksY; ++cy)
    {
        for (int cx = 0; cx < numChunksX; ++cx)
        {
            // Grab chunk ID
            glm::ivec2 chunkID = {cx, cy};

            // Create the chunk object as a child object
            auto& chunkObj = scene.CreateObject2D();
            object->AddChild(chunkObj);

            // Add the chunk object to the map
            m_chunkMap[chunkID] = &chunkObj;

            // Set transform offset
            int xoffset = cx * CHUNK_SIZE;
            int yoffset = cy *  CHUNK_SIZE;
            auto* pTransform = chunkObj.GetComponent<wolf::Transform2D>();
            pTransform->SetPosition(glm::vec2(xoffset * LABYRINTH_TILE_SIZE, yoffset * LABYRINTH_TILE_SIZE));

            // Create tilemap
            auto& tilemap = chunkObj.AddComponent<wolf::TileMap>(CHUNK_SIZE, CHUNK_SIZE);
            tilemap.LoadTileSet("data/labyrinth.tileset");

            // Iterate chunk's tilemap
            for (int y = 0; y < CHUNK_SIZE; ++y)
            {
                for (int x = 0; x < CHUNK_SIZE; ++x)
                {
                    // Calculate world position
                    glm::ivec2 worldPos = {x + xoffset, y + yoffset};

                    // Don't bother trying to place tiles that don't exist
                    if (worldPos.x >= m_width || worldPos.y >= m_height) continue;

                    // Get the logical tile at the current position
                    const LogicalTile& logicalTile = labyrinthGrid.Get(worldPos.x, worldPos.y);

                    // Convert from logical tile to specific tile ID
                    int tile = wolf::TileMap::EMPTY_TILE;
                    switch (logicalTile)
                    {
                        case LogicalTile::Unvisited:

                            // TESTING: For now display unvisited as floors
                            tile = Tile::FloorSmallSquares;
                            break;
                        
                        case LogicalTile::Door:
                            
                            // TODO: Choose which floor tile represents a door
                            tile = Tile::FloorSquareGold;
                            break;
                        
                        case LogicalTile::Floor:
                            
                            // TODO: Choose a random non-gold floor tile
                            tile = m_rng.NextInt(Tile::FloorSmallSquares, Tile::FloorSquare);
                            break;
                        
                        case LogicalTile::Grass:
                            tile = Tile::Grass;
                            break;
                        
                        case LogicalTile::Wall:

                            // TODO: Determine correct wall type based on surrounding tiles
                            tile = m_rng.NextInt(Tile::WallBottomLeft, Tile::WallTop);

                            // TODO: Add wall tile to wall collider for this chunk?
                            break;
                    }

                    tilemap.SetTile(x, y, tile);
                }
            }
        }
    }
}

void LabyrinthManager::DestroyLabyrinth()
{
    // Delete all child objects ob the labyrinth manager
    auto* object = GetGameObject();
    if (object)
    {
        object->DeleteAllChildren();
    }

    // Clear the chunk map
    m_chunkMap.clear();
}

void LabyrinthManager::Regenerate()
{
    DestroyLabyrinth();
    GenerateLabyrinth();
}

void LabyrinthManager::ShowGUI()
{
    // Setup window flags
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar;

    // Set window position and size
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({256, 512});
    ImGui::Begin("Daedalus' Terminal v0.1", nullptr, flags);

    // Menu bar for saving / loading labyrinth configs
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE_CIRCLE_PLUS " New"))
            {
                // TODO: Reset to default parameters
            }

            if (ImGui::MenuItem(ICON_FA_FILE " Load"))
            {
                // TODO: Load labyrinth config file from disk
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save"))
            {
                // TODO: Save labyrinth config file to disk
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save As..."))
            {
                // TODO: Save labyrinth config file to disk
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }

    ImGui::SeparatorText("Labyrinth Properties");

    // Property editors
    ImGui::Checkbox("Randomize Seed", &m_randomizeSeed);
    if (!m_randomizeSeed)
    {
        int seed = (int)m_rng.GetSeed();
        int prevSeed = seed;
        ImGui::InputInt("Seed", &seed);
        if (prevSeed != seed) m_rng.SetSeed(seed);
    }
    ImGui::DragInt("Width", &m_width, 1.0f, MIN_LABYRINTH_DIM, MAX_LABYRINTH_DIM);
    ImGui::DragInt("Height", &m_height, 1.0f, MIN_LABYRINTH_DIM, MAX_LABYRINTH_DIM);

    ImGui::SeparatorText("Rooms");

    // Adds a new room to the labyrinth
    if (ImGui::Button("Add Room")) m_rooms.push_back(Room());

    // Displays an editor for all rooms
    for (int i = 0; i < m_rooms.size(); ++i)
    {
        // Grab a reference to the current room
        Room& room = m_rooms[i];

        // Create dropdown header for each room
        bool keepRoom = true;

        ImGui::PushID(&room);
        if (ImGui::CollapsingHeader(("Room " + std::to_string(i) + "###").c_str(), &keepRoom, ImGuiTreeNodeFlags_None))
        {
            // Edit origin and size
            ImGui::DragInt2("Origin", &room.m_bounds.m_origin.x, 1.0f, 1, glm::max(m_width, m_height));
            ImGui::DragInt("Width", &room.m_bounds.m_size.x, 1.0f, 1, Room::MAX_SIZE);
            ImGui::DragInt("Height", &room.m_bounds.m_size.y, 1.0f, 1, Room::MAX_SIZE);
        }
        ImGui::PopID();

        // Delete room if requested
        if (!keepRoom)
        {
            m_rooms.erase(m_rooms.begin() + i);
            i--;
            keepRoom = true;
        }
    }

    ImGui::SeparatorText("Controls");

    // Buttons to destroy / regenerate the labyrinth
    if (ImGui::Button("Destroy")) DestroyLabyrinth();
    ImGui::SameLine();
    if (ImGui::Button("Regenerate")) Regenerate();

    // End of window
    ImGui::End();
}