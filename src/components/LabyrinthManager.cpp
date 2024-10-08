#include "LabyrinthManager.h"

// ImGui for GUI windows
#include <imgui/imgui.h>

// Icon font definitions
#include <IconsFontAwesome6.h>

#include <W_Logging.h>
#include <W_TileMap.h>
#include <W_Transform2D.h>

// For std::shuffle
#include <algorithm>

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

    // Ensure width and height are odd
    m_width = m_width % 2 == 0 ? m_width - 1 : m_width;
    m_height = m_height % 2 == 0 ? m_height - 1 : m_height;

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
    // wolf::Grid2D<unsigned int> dirGrid(m_width / 2, m_height / 2, 0);

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
    std::vector<wolf::IRectangle> placedRoomRects;
    for (int i = 0; i < m_rooms.size(); ++i)
    {
        // Grab references to the current room
        auto& room = m_rooms[i];
        auto& rect = room.m_bounds;

        // Try to place all instances of the room
        for (int instance = 0; instance < room.m_instances; ++instance)
        {
            // Generate final room properties

            // Room size
            switch (room.m_sizeType)
            {
                case Room::SizeType::Manual:
                    // Use bounds rectangle
                    break;
                
                case Room::SizeType::RandomMinMax:
                    
                    // Generate a random size between the min and max
                    rect.m_size.x = m_rng.NextInt(room.m_minSize.x, room.m_maxSize.x);
                    rect.m_size.y = m_rng.NextInt(room.m_minSize.y, room.m_maxSize.y);
                    break;
            }

            // Round down size if even (align with walls)
            rect.m_size.x = rect.m_size.x % 2 == 0 ? rect.m_size.x - 1 : rect.m_size.x;
            rect.m_size.y = rect.m_size.y % 2 == 0 ? rect.m_size.y - 1 : rect.m_size.y;

            // Flag to know if we found a valid position
            bool overlapping = false;

            // Room position (origin at bottom-left tile)
            switch (room.m_positionType)
            {
                case Room::PositionType::Manual:

                    // Use bounds rectangle
                    // Round down position if odd (align with walls)
                    rect.m_origin.x = rect.m_origin.x % 2 == 0 ? rect.m_origin.x - 1 : rect.m_origin.x;
                    rect.m_origin.y = rect.m_origin.y % 2 == 0 ? rect.m_origin.y - 1 : rect.m_origin.y;
                    break;
                
                case Room::PositionType::Random:

                    // Attempt to generate a valid position
                    for (int attempt = 0; attempt < Room::MAX_PLACEMENT_ATTEMPTS; ++attempt)
                    {
                        // Generate random position so that it is guaranteed in-bounds
                        rect.m_origin.x = m_rng.NextInt(1, m_width - rect.m_size.x - 1);
                        rect.m_origin.y = m_rng.NextInt(1, m_height - rect.m_size.y - 1);

                        // Round down position if odd (align with walls)
                        rect.m_origin.x = rect.m_origin.x % 2 == 0 ? rect.m_origin.x - 1 : rect.m_origin.x;
                        rect.m_origin.y = rect.m_origin.y % 2 == 0 ? rect.m_origin.y - 1 : rect.m_origin.y;

                        // Reset flag
                        overlapping = false;

                        // Don't bother trying to fix overlap on the final attempt if force is checked
                        if (attempt == Room::MAX_PLACEMENT_ATTEMPTS - 1 && room.m_force) break;

                        // Validate that the room wouldn't overlap anything
                        for (const auto& placedRect : placedRoomRects)
                        {
                            if (rect.Intersects(placedRect))
                            {
                                overlapping = true;
                                break;
                            }
                        }

                        // Exit if we found a valid spot
                        if (!overlapping) break;
                    }
                    break;
                
                case Room::PositionType::RandomRadius:

                    // Attempt to generate a valid position
                    for (int attempt = 0; attempt < Room::MAX_PLACEMENT_ATTEMPTS; ++attempt)
                    {
                        // Generate random position in a square "radius" around a position
                        const auto& pos = room.m_randomRadiusPosition;
                        const auto& r = room.m_randomRadius;
                        rect.m_origin.x = m_rng.NextInt(pos.x - r, pos.x + r);
                        rect.m_origin.y = m_rng.NextInt(pos.y - r, pos.y + r);

                        // Round down position if odd (align with walls)
                        rect.m_origin.x = rect.m_origin.x % 2 == 0 ? rect.m_origin.x - 1 : rect.m_origin.x;
                        rect.m_origin.y = rect.m_origin.y % 2 == 0 ? rect.m_origin.y - 1 : rect.m_origin.y;

                        // Ensure valid position
                        if (rect.m_origin.x > 0 && rect.m_origin.x < m_width - rect.m_size.x &&
                            rect.m_origin.y > 0 && rect.m_origin.y < m_height - rect.m_size.y)
                        {
                            // Reset flag
                            overlapping = false;

                            // Don't bother trying to fix overlap on the final attempt if force is checked
                            if (attempt == Room::MAX_PLACEMENT_ATTEMPTS - 1 && room.m_force) break;

                            // Check if overlapping
                            for (const auto& placedRect : placedRoomRects)
                            {
                                if (rect.Intersects(placedRect))
                                {
                                    overlapping = true;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            // Out of bounds, continue

                            // If last attempt, make sure to guarantee an in-bounds origin
                            if (attempt == Room::MAX_PLACEMENT_ATTEMPTS - 1 && room.m_force)
                            {
                                overlapping = false;
                                rect.m_origin.x = 1;
                                rect.m_origin.y = 1;
                            }

                            continue;
                        }

                        // Exit if we found a valid spot
                        if (!overlapping) break;
                    }
                    break;
            }

            // If we haven't found a valid position, don't place the room / instance
            if (overlapping)
            {
                wolf::Warning("Couldn't place room '", room.m_name.c_str(), "'");
                continue;
            }

            // If we reach here, must be non-overlapping, place the room!
            for (int y = -1; y <= rect.m_size.y; ++y)
            {
                for (int x = -1; x <= rect.m_size.x; ++x)
                {
                    glm::ivec2 worldPos = {x + rect.m_origin.x, y + rect.m_origin.y};

                    // Debug bounds checking
                    if (worldPos.x >= m_width || worldPos.y >= m_height || worldPos.x < 0 || worldPos.y < 0)
                    {
                        wolf::Warning("Room #", i, ", Tile (", worldPos.x, ", ", worldPos.y, ") out of bounds!");
                        continue;
                    }

                    // Set border tiles of each room as walls
                    if (x == -1 || x == rect.m_size.x || y == -1 || y == rect.m_size.y)
                    {
                        labyrinthGrid.Set(worldPos.x, worldPos.y, LogicalTile::Wall);
                        continue;
                    }

                    labyrinthGrid.Set(worldPos.x, worldPos.y, LogicalTile::Floor);
                }
            }

            // Add the specific room that was generated to the list
            placedRoomRects.push_back(rect);
        }
    }

    // Backtracking maze generation

    // Direction enum
    enum class Dir
    {
        N,
        E,
        S,
        W,
    };

    std::vector<Dir> directions =
    {
        Dir::N,
        Dir::E,
        Dir::S,
        Dir::W,
    };

    int dx[] =
    {
        0, 1, 0, -1
    };

    int dy[] =
    {
        1, 0, -1, 0
    };

    // Recursive lambda
    std::function<void(int, int)> CarvePassages = [&, this](int x, int y) -> void
    {
        // Shuffle the 4 directions
        auto dirs = directions;
        std::shuffle(dirs.begin(), dirs.end(), this->m_rng.GetEngine());

        // Place floor
        labyrinthGrid.Set(x, y, LogicalTile::Floor);

        for (Dir d : dirs)
        {
            // Calculate new coordinates
            int newX = dx[(int)d] * 2 + x;
            int newY = dy[(int)d] * 2 + y;

            // Bounds checking
            if (newX > 0 && newX < this->m_width - 1 && newY > 0 && newY < this->m_height - 1)
            {
                if (labyrinthGrid.Get(newX, newY) == LogicalTile::Unvisited)
                {
                    // Place floors
                    labyrinthGrid.Set(x + dx[(int)d], y + dy[(int)d], LogicalTile::Floor);

                    // Place walls
                    int wx = x + dx[((int)d + 1) % 4];
                    int wy = y + dy[((int)d + 1) % 4];
                    if (labyrinthGrid.Get(wx, wy) == LogicalTile::Unvisited) labyrinthGrid.Set(wx, wy, LogicalTile::Wall);
                    wx += dx[(int)d];
                    wy += dy[(int)d];
                    if (labyrinthGrid.Get(wx, wy) == LogicalTile::Unvisited) labyrinthGrid.Set(wx, wy, LogicalTile::Wall);
                    wx = x + dx[((int)d + 3) % 4];
                    wy = y + dy[((int)d + 3) % 4];
                    if (labyrinthGrid.Get(wx, wy) == LogicalTile::Unvisited) labyrinthGrid.Set(wx, wy, LogicalTile::Wall);
                    wx += dx[(int)d];
                    wy += dy[(int)d];
                    if (labyrinthGrid.Get(wx, wy) == LogicalTile::Unvisited) labyrinthGrid.Set(wx, wy, LogicalTile::Wall);

                    CarvePassages(newX, newY);
                }
            }
        }
    };

    CarvePassages(1, 1);

    // Generate all chunks

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
                            
                            // Do nothing
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

    // TODO: Separate procedural room parameters and custom rooms

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
        if (ImGui::CollapsingHeader((room.m_name + "###").c_str(), &keepRoom, ImGuiTreeNodeFlags_None))
        {
            ImGui::InputText("Name", &room.m_name);
            ImGui::DragInt("Instances", &room.m_instances, 1.0f, 1, 1024);
            ImGui::Checkbox("Force Generation", &room.m_force);

            ImGui::Separator();
            ImGui::Text("Position");

            // Edit position type
            const char* selectedPositionType = Room::s_positionTypeNames[(int)room.m_positionType];
            if (ImGui::BeginCombo("Type##position", selectedPositionType))
            {
                for (int n = 0; n < IM_ARRAYSIZE(Room::s_positionTypeNames); n++)
                {
                    bool is_selected = (selectedPositionType == Room::s_positionTypeNames[n]);
                    if (ImGui::Selectable(Room::s_positionTypeNames[n], is_selected))
                    {
                        room.m_positionType = (Room::PositionType)n;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            switch (room.m_positionType)
            {
                case Room::PositionType::Manual:

                    // Manually edit origin of room
                    ImGui::DragInt2("Origin", &room.m_bounds.m_origin.x, 1.0f, 1, glm::max(m_width, m_height));
                    break;
                
                case Room::PositionType::Random:

                    // Truly random, no need for action
                    break;
                
                case Room::PositionType::RandomRadius:

                    // Edit origin and radius for random position
                    ImGui::DragInt2("Position", &room.m_randomRadiusPosition.x, 1.0f, 1, glm::max(m_width, m_height));
                    ImGui::DragInt("Radius", &room.m_randomRadius, 1.0f, 1, INT32_MAX);
                    break;
            }

            ImGui::Separator();
            ImGui::Text("Size");

            // Edit size type
            const char* selectedSizeType = Room::s_sizeTypeNames[(int)room.m_sizeType];
            if (ImGui::BeginCombo("Type##size", selectedSizeType))
            {
                for (int n = 0; n < IM_ARRAYSIZE(Room::s_sizeTypeNames); n++)
                {
                    bool is_selected = (selectedSizeType == Room::s_sizeTypeNames[n]);
                    if (ImGui::Selectable(Room::s_sizeTypeNames[n], is_selected))
                    {
                        room.m_sizeType = (Room::SizeType)n;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            switch (room.m_sizeType)
            {
                case Room::SizeType::Manual:

                    // Manually edit the width and height of the room
                    ImGui::DragInt2("Size", &room.m_bounds.m_size.x, 1.0f, 1, glm::max(m_width, m_height));
                    break;
                
                case Room::SizeType::RandomMinMax:

                    // Edit the minimum and maximum size of the room
                    ImGui::DragInt2("Min", &room.m_minSize.x, 1.0f, 1, glm::max(m_width, m_height));
                    ImGui::DragInt2("Max", &room.m_maxSize.x, 1.0f, 1, glm::max(m_width, m_height));
                    break;
            }

            ImGui::Separator();
            ImGui::Text("Entities");

            if (ImGui::Button("Add Entity"))
            {
                // TODO: Add entity types dropdown
            }
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