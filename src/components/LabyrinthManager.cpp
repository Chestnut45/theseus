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

// For portable file paths
#include <filesystem>
#include <fstream>

// Platform native file dialog helper
#include <portable-file-dialogs.h>

// For parsing the labyrinth config file
#include <yaml-cpp/yaml.h>

#include <ChestInventoryComponent.h>
#include <DispensaryInventoryComponent.h>
#include <ColliderComponent.h>
#include <EnemyDataLoader.h>
#include <MinitaurBuilder.h>
#include <HarpyBuilder.h>
#include <GorgonBuilder.h>
#include <PlayerController.h>
#include <TriggerComponent.h>

std::unordered_map<std::string, LabyrinthManager::Room::EntityType> LabyrinthManager::s_entityIDs;

LabyrinthManager::LabyrinthManager()
{
    s_entityIDs["minitaur"] = Room::EntityType::Minitaur;
    s_entityIDs["harpy"] = Room::EntityType::Harpy;
    s_entityIDs["gorgon"] = Room::EntityType::Gorgon;
    s_entityIDs["common_chest"] = Room::EntityType::CommonChest;
    s_entityIDs["uncommon_chest"] = Room::EntityType::UncommonChest;
    s_entityIDs["rare_chest"] = Room::EntityType::RareChest;
    s_entityIDs["epic_chest"] = Room::EntityType::EpicChest;
    s_entityIDs["legendary_chest"] = Room::EntityType::LegendaryChest;
    s_entityIDs["dispensary"] = Room::EntityType::DaedalusDispensary;
    s_entityIDs["spike_trap"] = Room::EntityType::SpikeTrap;
}

LabyrinthManager::~LabyrinthManager()
{
}

void LabyrinthManager::Update(float delta)
{
    auto* pObject = GetGameObject();
    auto* pPlayer = GetPlayer();

    // If player is not in scene, no need to update
    if (!pPlayer) return;

    // Grab global position of the player and get current chunk
    auto* pPlayerTransform = pPlayer->GetComponent<wolf::Transform2D>();
    auto worldPos = pPlayerTransform->GetGlobalPosition();
    auto chunkID = GetChunkID(worldPos);

    if (chunkID != m_prevChunk)
    {
        // Chunk has changed
        glm::ivec2 chunksToLoad[] =
        {
            chunkID,
            chunkID + glm::ivec2(0, 1),
            chunkID + glm::ivec2(1, 0),
            chunkID + glm::ivec2(1, 1),
            chunkID + glm::ivec2(0, -1),
            chunkID + glm::ivec2(-1, 0),
            chunkID + glm::ivec2(-1, -1),
            chunkID + glm::ivec2(1, -1),
            chunkID + glm::ivec2(-1, 1),
        };

        // Update queues
        for (const auto& id : chunksToLoad)
        {
            // Don't bother processing a chunk that doesn't exist
            if (!m_chunkMap.contains(id)) continue;

            // Check if already active
            const auto& chunkData = m_chunkMap[id];
            if (chunkData.active) continue;

            // Not active, add to queue if not already there
            bool queued = false;
            for (const auto& c : m_chunkActivateQueue)
            {
                if (id == c)
                {
                    queued = true;
                    break;
                }
            }
            if (!queued) m_chunkActivateQueue.push_back(id);

            // Ensure it's not on the deactivate queue
            for (int i = m_chunkDeactivateQueue.size() - 1; i >= 0; --i)
            {
                if (m_chunkDeactivateQueue[i] == id)
                {
                    m_chunkDeactivateQueue.erase(m_chunkDeactivateQueue.begin() + i);
                    break;
                }
            }
        }

        for (const auto& chunk : m_chunkMap)
        {
            if (chunk.second.active)
            {
                bool shouldBeActive = false;
                for (const auto& id : chunksToLoad)
                {
                    if (id == chunk.first)
                    {
                        shouldBeActive = true;
                        break;
                    }
                }
                if (!shouldBeActive) m_chunkDeactivateQueue.push_back(chunk.first);
            }
        }
    }

    // Process one chunk per frame from each queue
    int next = m_chunkActivateQueue.size() - 1;
    if (next >= 0)
    {
        ActivateChunk(m_chunkActivateQueue[next]);
        m_chunkActivateQueue.pop_back();
    }

    next = m_chunkDeactivateQueue.size() - 1;
    if (next >= 0)
    {
        DeactivateChunk(m_chunkDeactivateQueue[next]);
        m_chunkDeactivateQueue.pop_back();
    }

    // Update cached chunk ID
    m_prevChunk = chunkID;
}

void LabyrinthManager::ActivateChunk(const glm::ivec2& chunkID)
{
    const auto& it = m_chunkMap.find(chunkID);
    if (it == m_chunkMap.end()) return;

    // Return if chunk already active
    auto& chunk = it->second;
    if (chunk.active) return;

    std::function<void(wolf::GameObject*)> Activate = [&](wolf::GameObject* pObject) -> void
    {
        // Make tilemaps visible
        auto* pTileMap = pObject->GetComponent<wolf::TileMap>();
        if (pTileMap) pTileMap->SetVisibility(true);

        // Make sprites visible
        auto* pSprite = pObject->GetComponent<AnimatedSprite2D>();
        if (pSprite) pSprite->SetVisibility(true);

        // Activate collider
        // TODO: Only do this for walls? Or Move enemies to different chunks...
        auto* pCollider = pObject->GetComponent<ColliderComponent>();
        if (pCollider) pCollider->SetActive(true);

        // Recursively activate all child objects and compatible components
        for (auto* pChild : pObject->GetChildren())
        {
            Activate(pChild);
        }
    };
    
    // Activate the object hierarchy
    Activate(chunk.m_pObject);
    chunk.active = true;
}

void LabyrinthManager::DeactivateChunk(const glm::ivec2& chunkID)
{
    const auto& it = m_chunkMap.find(chunkID);
    if (it == m_chunkMap.end()) return;

    // Return if chunk already inactive
    auto& chunk = it->second;
    if (!chunk.active) return;

    std::function<void(wolf::GameObject*)> Deactivate = [&](wolf::GameObject* pObject) -> void
    {
        // Make tilemaps invisible
        auto* pTileMap = pObject->GetComponent<wolf::TileMap>();
        if (pTileMap) pTileMap->SetVisibility(false);

        // Make sprites invisible
        auto* pSprite = pObject->GetComponent<AnimatedSprite2D>();
        if (pSprite) pSprite->SetVisibility(false);

        // Deactivate collider
        // TODO: Only do this for walls? Or Move enemies to different chunks...
        auto* pCollider = pObject->GetComponent<ColliderComponent>();
        if (pCollider) pCollider->SetActive(false);

        // Recursively deactivate all child objects and compatible components
        for (auto* pChild : pObject->GetChildren())
        {
            Deactivate(pChild);
        }
    };
    
    // Deactivate the object hierarchy
    Deactivate(chunk.m_pObject);
    chunk.active = false;
}

void LabyrinthManager::GenerateLabyrinth()
{
    // Only bother if valid generation parameters
    if (m_width <= 0 || m_height <= 0)
    {
        wolf::Error("Can't generate labyrinth, invalid width / height");
        return;
    }

    auto* pObject = GetGameObject();
    if (!pObject)
    {
        wolf::Error("Labyrinth generator not attached to a GameObject");
        return;
    }

    // Clear all data structures
    m_tileSectionMap.clear();
    m_sections.clear();
    m_prevChunk = glm::ivec2(0);

    // Ensure width and height are odd
    m_width = m_width % 2 == 0 ? m_width - 1 : m_width;
    m_height = m_height % 2 == 0 ? m_height - 1 : m_height;

    // Ensure spawn room size is odd
    m_spawnRoomSize.x = m_spawnRoomSize.x % 2 == 0 ? m_spawnRoomSize.x - 1 : m_spawnRoomSize.x;
    m_spawnRoomSize.y = m_spawnRoomSize.y % 2 == 0 ? m_spawnRoomSize.y - 1 : m_spawnRoomSize.y;

    // Initialize scale of all labyrinth objects
    auto* pTransform = pObject->GetComponent<wolf::Transform2D>();
    if (!pTransform) pTransform = &pObject->AddComponent<wolf::Transform2D>();

    // Reseed the rng before generating
    if (m_randomizeSeed) m_rng.SetSeed(m_rng.NextInt(0, INT32_MAX));
    else m_rng.Reseed();

    // Initialize global grid of logical tile data for entire labyrinth
    m_labyrinthGrid.Resize(m_width, m_height, LogicalTile::Unvisited);

    // Place top/bottom outer walls
    for (int i = 0; i < m_width; ++i)
    {
        m_labyrinthGrid.Set(i, 0, LogicalTile::Wall);
        m_labyrinthGrid.Set(i, m_height - 1, LogicalTile::Wall);
    }

    // Place left/right outer walls
    for (int i = 0; i < m_height; ++i)
    {
        m_labyrinthGrid.Set(0, i, LogicalTile::Wall);
        m_labyrinthGrid.Set(m_width - 1, i, LogicalTile::Wall);
    }

    // Place all rooms into the logical tilemap
    auto placedRooms = PlaceRooms();

    // Carve maze into the logical tilemap
    CarveMaze();

    // Guarantee connectivity of all sections to the entrance
    ConnectRooms(placedRooms);

    // Open up the labyrinth entrance tiles
    m_labyrinthGrid.Set(m_width / 2, 0, LogicalTile::Floor);
    m_labyrinthGrid.Set(m_width / 2, 1, LogicalTile::Floor);

    // Convert the logical tilemap into chunks and objects
    GenerateChunks();

    // Place all entitites
    PopulateEntities(placedRooms);

    // Deactivate all chunks
    for (auto& chunk : m_chunkMap)
    {
        DeactivateChunk(chunk.first);
    }

    // Flag has to be updated first so that
    // GetSpawnLocation() can be called by GenerateEntrance()
    m_isGenerated = true;

    // Generate entrance room
    GenerateEntrance();

    // Place the player at the spawn location of the labyrinth
    wolf::GameObject* pPlayer = nullptr;
    for (auto&&[_, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        // Position player at spawn
        pPlayer = playerController.GetGameObject();
        pTransform = pPlayer->GetComponent<wolf::Transform2D>();
        pTransform->SetPosition(GetSpawnLocation());

        // Update camera position
        auto* pCamera = pPlayer->GetChildren()[0]->GetComponent<wolf::Camera2D>();
        if (pCamera) pCamera->SetPosition(pTransform->GetLocalPosition());
        break;
    }
}

void LabyrinthManager::DestroyLabyrinth()
{
    // Delete all child objects of the labyrinth manager
    auto* object = GetGameObject();
    if (object)
    {
        object->DeleteAllChildren();
    }

    // Clear the chunk map
    m_chunkMap.clear();

    // Update flag
    m_isGenerated = false;
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
    ImGui::SetNextWindowSize({320, 512});
    ImGui::Begin("Daedalus' Terminal v0.1", nullptr, flags);

    // Menu bar for saving / loading labyrinth configs
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE_CIRCLE_PLUS " New"))
            {
                Reset();
            }

            if (ImGui::MenuItem(ICON_FA_FILE " Load..."))
            {
                auto file = pfd::open_file("Load Labyrinth Config", (std::filesystem::current_path() / "data").generic_string(), {"YAML configs (.yaml)", "*.yaml"}, pfd::opt::none);
                if (file.result().size() > 0)
                {
                    // Grab the generic portable version of the path
                    auto path = std::filesystem::path(file.result()[0]).generic_string();
                    LoadConfig(path);
                }
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save..."))
            {
                auto file = pfd::save_file("Save Labyrinth Config", (std::filesystem::current_path() / "data").generic_string(), {"YAML configs (.yaml)", "*.yaml"}, pfd::opt::none);
                if (file.result().size() > 0)
                {
                    auto path = std::filesystem::path(file.result()).generic_string();
                    SaveConfig(path);
                }
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
                room.m_entitySpawns.push_back(Room::EntitySpawnData());
            }

            // Iterate all entity spawn data
            for (int e = 0; e < room.m_entitySpawns.size(); ++e)
            {
                bool keep = true;

                // Grab entity spawn data
                auto& entityData = room.m_entitySpawns[e];

                // Push the address as an identifier
                ImGui::PushID(&entityData);

                // Insert a separator between each entity spawn
                ImGui::Separator();

                // Remove the entity spawn if requested
                if (ImGui::Button("Remove")) keep = false;

                // Edit entity spawn type
                const char* selectedEntityType = Room::s_entityTypeNames[(int)entityData.m_type];
                if (ImGui::BeginCombo("Entity Type##entity", selectedEntityType))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(Room::s_entityTypeNames); n++)
                    {
                        bool is_selected = (selectedEntityType == Room::s_entityTypeNames[n]);
                        if (ImGui::Selectable(Room::s_entityTypeNames[n], is_selected))
                        {
                            entityData.m_type = (Room::EntityType)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Edit entity spawn position type
                const char* selectedSpawnPosType = Room::s_entitySpawnPosNames[(int)entityData.m_spawnPosType];
                if (ImGui::BeginCombo("Placement##entity", selectedSpawnPosType))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(Room::s_entitySpawnPosNames); n++)
                    {
                        bool is_selected = (selectedSpawnPosType == Room::s_entitySpawnPosNames[n]);
                        if (ImGui::Selectable(Room::s_entitySpawnPosNames[n], is_selected))
                        {
                            entityData.m_spawnPosType = (Room::SpawnPosType)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if (entityData.m_spawnPosType == Room::SpawnPosType::Manual)
                {
                    ImGui::DragInt2("Position", &entityData.m_pos.x);
                }

                // Edit amount of spawns
                ImGui::DragInt("Amount", &entityData.m_amount, 1.0f, 1, 64);

                ImGui::PopID();

                if (!keep)
                {
                    room.m_entitySpawns.erase(room.m_entitySpawns.begin() + e);
                    e--;
                }
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

void LabyrinthManager::LoadConfig(const std::string& filepath)
{
    Reset();
    try
    {
        // Load the YAML file as a node
        YAML::Node node = YAML::LoadFile(filepath);

        // Load labyrinth properties
        m_randomizeSeed = node["random_seed"] ? node["random_seed"].as<bool>() : m_randomizeSeed;
        if (node["seed"]) m_rng.SetSeed(node["seed"].as<int>());
        m_width = node["width"] ? node["width"].as<int>() : m_width;
        m_height = node["height"] ? node["height"].as<int>() : m_height;

        // Load room data
        YAML::Node rooms = node["rooms"];
        for (int i = 0; i < rooms.size(); ++i)
        {
            // Grab the specific room node
            YAML::Node r = rooms[i];

            // Start loading room properties
            Room room;
            room.m_name = r["name"] ? r["name"].as<std::string>() : room.m_name;
            room.m_instances = r["instances"] ? r["instances"].as<int>() : room.m_instances;
            room.m_force = r["force"] ? r["force"].as<bool>() : room.m_force;
            
            // Parse position data
            YAML::Node pos = r["position"];
            std::string posType = pos["type"].as<std::string>();
            if (posType == "manual")
            {
                room.m_positionType = Room::PositionType::Manual;
                room.m_bounds.m_origin.x = pos["origin"]["x"].as<int>();
                room.m_bounds.m_origin.y = pos["origin"]["y"].as<int>();
            }
            else if (posType == "random")
            {
                room.m_positionType = Room::PositionType::Random;
            }
            else if (posType == "random_radius")
            {
                room.m_positionType = Room::PositionType::RandomRadius;
                room.m_randomRadiusPosition.x = pos["position"]["x"].as<int>();
                room.m_randomRadiusPosition.y = pos["position"]["y"].as<int>();
                room.m_randomRadius = pos["radius"].as<int>();
            }

            // Parse size data
            YAML::Node size = r["size"];
            std::string sizeType = size["type"].as<std::string>();
            if (sizeType == "manual")
            {
                room.m_sizeType = Room::SizeType::Manual;
                room.m_bounds.m_size.x = size["value"]["x"].as<int>();
                room.m_bounds.m_size.y = size["value"]["y"].as<int>();
            }
            else if (sizeType == "random_min_max")
            {
                room.m_sizeType = Room::SizeType::RandomMinMax;
                room.m_minSize.x = size["min"]["x"].as<int>();
                room.m_minSize.y = size["min"]["y"].as<int>();
                room.m_maxSize.x = size["max"]["x"].as<int>();
                room.m_maxSize.y = size["max"]["y"].as<int>();
            }

            // Parse entity data
            // TODO: Add more entity types
            YAML::Node entities = r["entities"];
            for (int e = 0; e < entities.size(); ++e)
            {
                Room::EntitySpawnData data;

                // Grab the entity node
                YAML::Node entity = entities[e];
                std::string eType = entity["type"] ? entity["type"].as<std::string>() : "";
                
                // Grab entity ID from map
                data.m_type = s_entityIDs[eType];
                data.m_amount = entity["amount"] ? entity["amount"].as<int>() : data.m_amount;

                // Parse placement
                std::string ePlacement = entity["placement"] ? entity["placement"].as<std::string>() : "";
                if (ePlacement == "center") data.m_spawnPosType = Room::SpawnPosType::Center;
                if (ePlacement == "manual") data.m_spawnPosType = Room::SpawnPosType::Manual;
                if (ePlacement == "random") data.m_spawnPosType = Room::SpawnPosType::Random;
                if (entity["position"])
                {
                    data.m_pos.x = entity["position"]["x"].as<int>();
                    data.m_pos.y = entity["position"]["y"].as<int>();
                }

                // Add the spawn data to the current room
                room.m_entitySpawns.push_back(data);
            }

            // Add the room to the list of rooms
            m_rooms.push_back(room);
        }
    }
    catch (YAML::Exception& e)
    {
        wolf::Error("Error parsing file '", filepath.c_str(), "': ", e.what());
    }
}

void LabyrinthManager::SaveConfig(const std::string& filepath)
{
    std::ofstream file(filepath, std::ios::binary);

    file << "random_seed: ";
    if (m_randomizeSeed) file << "true\n";
    else file << "false\n";

    file << "seed: ";
    file << std::to_string(m_rng.GetSeed()).c_str();
    file << "\n";

    file << "width: " << std::to_string(m_width).c_str();
    file << "\n";

    file << "height: " << std::to_string(m_height).c_str();
    file << "\n\n";

    file << "rooms: [\n";

    for (int i = 0; i < m_rooms.size(); ++i)
    {
        const auto& room = m_rooms[i];
        file << "\t{\n";

        file << "\t\tname: ";
        file << room.m_name.c_str();
        file << ",\n";

        file << "\t\tinstances: ";
        file << std::to_string(room.m_instances).c_str();
        file << ",\n";

        file << "\t\tforce: ";
        if (room.m_force) file << "true,\n";
        else file << "false,\n";

        // Output position data
        file << "\t\tposition: {";
        file << "type: ";
        switch (room.m_positionType)
        {
            case Room::PositionType::Manual:
                file << "manual, origin: {x: ";
                file << std::to_string(room.m_bounds.m_origin.x).c_str();
                file << ", y: ";
                file << std::to_string(room.m_bounds.m_origin.y).c_str();
                file << "}";
                break;
            
            case Room::PositionType::Random:
                file << "random";
                break;
            
            case Room::PositionType::RandomRadius:
                file << "random_radius, position: {x: ";
                file << std::to_string(room.m_randomRadiusPosition.x).c_str();
                file << ", y: ";
                file << std::to_string(room.m_randomRadiusPosition.y).c_str();
                file << "}, radius: ";
                file << std::to_string(room.m_randomRadius).c_str();
                break;
        }
        file << "},\n";

        // Output size data
        file << "\t\tsize: {";
        file << "type: ";
        switch (room.m_sizeType)
        {
            case Room::SizeType::Manual:
                file << "manual, value: {x: ";
                file << std::to_string(room.m_bounds.m_size.x).c_str();
                file << ", y: ";
                file << std::to_string(room.m_bounds.m_size.y).c_str();
                file << "}";
                break;
            
            case Room::SizeType::RandomMinMax:
                file << "random_min_max, min: {x: ";
                file << std::to_string(room.m_minSize.x).c_str();
                file << ", y: ";
                file << std::to_string(room.m_minSize.y).c_str();
                file << "}, max: {x: ";
                file << std::to_string(room.m_maxSize.x).c_str();
                file << ", y: ";
                file << std::to_string(room.m_maxSize.y).c_str();
                file << "}";
                break;
        }
        file << "},\n";
        
        // Output entity data
        file << "\t\tentities: [\n";
        for (int e = 0; e < room.m_entitySpawns.size(); ++e)
        {
            const auto& data = room.m_entitySpawns[e];
            file << "\t\t\t{type: ";
            switch (data.m_type)
            {
                case Room::EntityType::Minitaur:
                    file << "minitaur, amount: ";
                    break;
                case Room::EntityType::Harpy:
                    file << "harpy, amount: ";
                    break;
                case Room::EntityType::Gorgon:
                    file << "gorgon, amount: ";
                    break;
                case Room::EntityType::CommonChest:
                    file << "common_chest, amount: ";
                    break;
                case Room::EntityType::UncommonChest:
                    file << "uncommon_chest, amount: ";
                    break;
                case Room::EntityType::RareChest:
                    file << "rare_chest, amount: ";
                    break;
                case Room::EntityType::EpicChest:
                    file << "epic_chest, amount: ";
                    break;
                case Room::EntityType::LegendaryChest:
                    file << "legendary_chest, amount: ";
                    break;
                case Room::EntityType::DaedalusDispensary:
                    file << "dispensary, amount: ";
                    break;
                case Room::EntityType::SpikeTrap:
                    file << "spike_trap, amount: ";
                    break;
            }
            file << std::to_string(data.m_amount).c_str();
            file << ", placement: ";
            switch (data.m_spawnPosType)
            {
                case Room::SpawnPosType::Center:
                    file << "center";
                    break;
                case Room::SpawnPosType::Manual:
                    file << "manual, position: {x: ";
                    file << std::to_string(data.m_pos.x);
                    file << ", y: ";
                    file << std::to_string(data.m_pos.y);
                    file << "}";
                    break;
                case Room::SpawnPosType::Random:
                    file << "random";
                    break;
            }
            file << "},\n";
        }
        file << "\t\t]\n";

        file << "\t}";

        // Add a comma to every room except the last one
        if (i != m_rooms.size() - 1)
            file << ",";
        
        file << "\n";
    }

    file << "]";
}

glm::vec2 LabyrinthManager::GetSpawnLocation() const
{
    if (!m_isGenerated) return glm::vec2(0.0f);
    return glm::vec2((float)m_width / 2 * TILE_SIZE * SCALE, -(float)m_spawnRoomSize.y / 2 * TILE_SIZE * SCALE);
}

glm::ivec2 LabyrinthManager::GetChunkID(const glm::vec2& worldPosition) const
{
    return worldPosition / glm::vec2(TILE_SIZE * CHUNK_SIZE * SCALE);
}

wolf::GameObject* LabyrinthManager::GetChunk(const glm::ivec2& chunkID) const
{
    const auto it = m_chunkMap.find(chunkID);
    if (it == m_chunkMap.end()) return nullptr;
    return it->second.m_pObject;
}

void LabyrinthManager::DeleteChunk(const glm::ivec2& chunkID)
{
    auto* pChunk = GetChunk(chunkID);
    if (pChunk)
    {
        pChunk->Delete();
        m_chunkMap.erase(chunkID);
    }
}

glm::ivec2 LabyrinthManager::GetTilePosition(const glm::vec2& worldPosition) const
{
    if (worldPosition.x < 0 || worldPosition.y < 0 || worldPosition.x >= m_width * SCALE * TILE_SIZE || worldPosition.y >= m_height * SCALE * TILE_SIZE)
    {
        return glm::ivec2(-1);
    }
    
    return worldPosition / glm::vec2(SCALE * TILE_SIZE);
}

int LabyrinthManager::GetTile(int x, int y) const
{
    // Validate position
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return -2;

    // Grab chunk pointer
    glm::ivec2 chunkID(x / CHUNK_SIZE, y / CHUNK_SIZE);
    auto* pChunk = GetChunk(chunkID);

    if (!pChunk)
    {
        wolf::Warning("Chunk does not exist in call to GetTile(...)");
        return -2;
    }

    auto* pTilemap = pChunk->GetChildren()[0]->GetComponent<wolf::TileMap>();
    if (!pTilemap)
    {
        wolf::Warning("No tilemap found in call to GetTile(...)");
        return -2;
    }

    glm::ivec2 localPos = glm::ivec2(x - (chunkID.x * CHUNK_SIZE), y - (chunkID.y * CHUNK_SIZE));
    return pTilemap->GetTile(localPos.x, localPos.y);
}

void LabyrinthManager::SetTile(int x, int y, int tileID)
{
    // Validate position
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return;

    // Grab chunk pointer
    glm::ivec2 chunkID(x / CHUNK_SIZE, y / CHUNK_SIZE);
    auto* pChunk = GetChunk(chunkID);

    if (!pChunk)
    {
        wolf::Warning("Chunk does not exist in call to SetTile(...)");
        return;
    }

    auto* pTilemap = pChunk->GetChildren()[0]->GetComponent<wolf::TileMap>();
    if (!pTilemap)
    {
        wolf::Warning("No tilemap found in call to SetTile(...)");
        return;
    }

    glm::ivec2 localPos = glm::ivec2(x - (chunkID.x * CHUNK_SIZE), y - (chunkID.y * CHUNK_SIZE));
    pTilemap->SetTile(localPos.x, localPos.y, tileID);
}

void LabyrinthManager::Reset()
{
    // Reset to default values
    m_randomizeSeed = false;
    m_rng.SetSeed(0);
    m_width = 125;
    m_height = 125;
    m_rooms.clear();
}

wolf::GameObject* LabyrinthManager::GetPlayer() const
{
    for (auto&&[_, playercontroller] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        return playercontroller.GetGameObject();
    }
    return nullptr;
}

std::vector<LabyrinthManager::Room> LabyrinthManager::PlaceRooms()
{
    // Place all rooms into the labyrinth
    std::vector<Room> placedRooms;
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
                        for (const auto& room : placedRooms)
                        {
                            if (rect.Intersects(room.m_bounds))
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
                            for (const auto& room : placedRooms)
                            {
                                if (rect.Intersects(room.m_bounds))
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

            // If we reach here, the room must be non-overlapping, place it!

            // Create a new logical section for connectivity rules
            int newSection = m_sections.size();
            m_sections.push_back(Section());

            // Iterate all tiles included in the room
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
                        m_labyrinthGrid.Set(worldPos.x, worldPos.y, LogicalTile::Wall);
                        continue;
                    }

                    // Set floor tiles
                    m_labyrinthGrid.Set(worldPos.x, worldPos.y, LogicalTile::Floor);

                    // Add floor tiles to new section
                    m_tileSectionMap[worldPos] = newSection;
                }
            }

            // Add the specific room that was generated to the list
            placedRooms.push_back(room);
        }
    }

    return placedRooms;
}

void LabyrinthManager::CarveMaze()
{
    // Backtracking maze generation
    // Adapted from the ruby implementation presented by Jamis Buck:
    // https://weblog.jamisbuck.org/2010/12/27/maze-generation-recursive-backtracking

    // TODO: Reimplement as an iterative version to remove the
    // limitation on labyrinth size due to stack frame size limits.

    enum class Dir
    {
        N,
        E,
        S,
        W,
    };

    int deltaDirX[] = { 0, 1, 0, -1 };
    int deltaDirY[] = { 1, 0, -1, 0 };

    // Recursive lambda function to carve the maze into the labyrinth
    std::function<void(int, int)> CarveMaze = [&, this](int x, int y) -> void
    {
        // Shuffle the 4 directions
        std::vector<Dir> dirs =
        {
            Dir::N,
            Dir::E,
            Dir::S,
            Dir::W,
        };
        std::shuffle(dirs.begin(), dirs.end(), this->m_rng.GetEngine());

        // Place floor so we know this tile has been visited
        m_labyrinthGrid.Set(x, y, LogicalTile::Floor);

        for (Dir d : dirs)
        {
            // Grab deltas
            int dx = deltaDirX[(int)d];
            int dy = deltaDirY[(int)d];

            // Calculate new coordinates
            int newX = dx * 2 + x;
            int newY = dy * 2 + y;

            // Bounds checking
            if (newX > 0 && newX < this->m_width - 1 && newY > 0 && newY < this->m_height - 1)
            {
                if (m_labyrinthGrid.Get(newX, newY) == LogicalTile::Unvisited)
                {
                    // Place intermediate floor
                    m_labyrinthGrid.Set(x + dx, y + dy, LogicalTile::Floor);

                    // Place walls
                    int wx = x + deltaDirX[((int)d + 1) % 4];
                    int wy = y + deltaDirY[((int)d + 1) % 4];
                    if (m_labyrinthGrid.Get(wx, wy) == LogicalTile::Unvisited) m_labyrinthGrid.Set(wx, wy, LogicalTile::Wall);
                    wx += dx;
                    wy += dy;
                    if (m_labyrinthGrid.Get(wx, wy) == LogicalTile::Unvisited) m_labyrinthGrid.Set(wx, wy, LogicalTile::Wall);
                    wx = x + deltaDirX[((int)d + 3) % 4];
                    wy = y + deltaDirY[((int)d + 3) % 4];
                    if (m_labyrinthGrid.Get(wx, wy) == LogicalTile::Unvisited) m_labyrinthGrid.Set(wx, wy, LogicalTile::Wall);
                    wx += dx;
                    wy += dy;
                    if (m_labyrinthGrid.Get(wx, wy) == LogicalTile::Unvisited) m_labyrinthGrid.Set(wx, wy, LogicalTile::Wall);

                    CarveMaze(newX, newY);
                }
            }
        }
    };

    // Carve mazes into every unvisited tile in the labyrinth
    for (int y = 1; y < m_height - 1; ++y)
    {
        for (int x = 1; x < m_width - 1; ++x)
        {
            if (m_labyrinthGrid.Get(x, y) == LogicalTile::Unvisited)
            {
                // Carve maze from this section
                CarveMaze(x, y);
            }
        }
    }

    // Cleanup passes

    // Single floor surrounded by walls fix
    for (int y = 1; y < m_height - 1; ++y)
    {
        for (int x = 1; x < m_width - 1; ++x)
        {
            if (m_labyrinthGrid.Get(x, y) == LogicalTile::Floor)
            {
                // Grab all neighbour values
                short up = m_labyrinthGrid.Get(x, y + 1) == LogicalTile::Wall;
                short down = m_labyrinthGrid.Get(x, y - 1) == LogicalTile::Wall;
                short left = m_labyrinthGrid.Get(x - 1, y) == LogicalTile::Wall;
                short right = m_labyrinthGrid.Get(x + 1, y) == LogicalTile::Wall;
                if (up + down + left + right == 4)
                {
                    m_labyrinthGrid.Set(x, y, LogicalTile::Wall);
                }
            }
        }
    }
}

void LabyrinthManager::ConnectRooms(const std::vector<LabyrinthManager::Room>& placedRooms)
{
    // Detect all maze sections
    std::vector<glm::ivec2> toProcess;
    for (int y = 1; y < m_height - 2; ++y)
    {
        for (int x = 1; x < m_width - 2; ++x)
        {
            if (m_labyrinthGrid.Get(x, y) == LogicalTile::Floor)
            {
                glm::ivec2 pos(x, y);
                if (!m_tileSectionMap.contains(pos))
                {
                    // Create new section
                    int newSection = m_sections.size();
                    m_sections.push_back(Section());

                    m_tileSectionMap[pos] = newSection;

                    if (m_labyrinthGrid.Get(x - 1, y) == LogicalTile::Floor && !m_tileSectionMap.contains(glm::ivec2(x - 1, y)))
                    {
                        toProcess.push_back(glm::ivec2(x - 1, y));
                    }
                    if (m_labyrinthGrid.Get(x + 1, y) == LogicalTile::Floor && !m_tileSectionMap.contains(glm::ivec2(x + 1, y)))
                    {
                        toProcess.push_back(glm::ivec2(x + 1, y));
                    }
                    if (m_labyrinthGrid.Get(x, y - 1) == LogicalTile::Floor && !m_tileSectionMap.contains(glm::ivec2(x, y - 1)))
                    {
                        toProcess.push_back(glm::ivec2(x, y - 1));
                    }
                    if (m_labyrinthGrid.Get(x, y + 1) == LogicalTile::Floor && !m_tileSectionMap.contains(glm::ivec2(x, y + 1)))
                    {
                        toProcess.push_back(glm::ivec2(x, y + 1));
                    }

                    while (toProcess.size() > 0)
                    {
                        const auto p = toProcess[toProcess.size() - 1];
                        toProcess.pop_back();
                        m_tileSectionMap[p] = newSection;
                        
                        if (m_labyrinthGrid.Get(p.x - 1, p.y) == LogicalTile::Floor && !m_tileSectionMap.contains(glm::ivec2(p.x - 1, p.y)))
                        {
                            toProcess.push_back(glm::ivec2(p.x - 1, p.y));
                        }
                        if (m_labyrinthGrid.Get(p.x + 1, p.y) == LogicalTile::Floor && !m_tileSectionMap.contains(glm::ivec2(p.x + 1, p.y)))
                        {
                            toProcess.push_back(glm::ivec2(p.x + 1, p.y));
                        }
                        if (m_labyrinthGrid.Get(p.x, p.y - 1) == LogicalTile::Floor && !m_tileSectionMap.contains(glm::ivec2(p.x, p.y - 1)))
                        {
                            toProcess.push_back(glm::ivec2(p.x, p.y - 1));
                        }
                        if (m_labyrinthGrid.Get(p.x, p.y + 1) == LogicalTile::Floor && !m_tileSectionMap.contains(glm::ivec2(p.x, p.y + 1)))
                        {
                            toProcess.push_back(glm::ivec2(p.x, p.y + 1));
                        }
                    }
                }
            }
        }
    }

    // Add all connectors between disconnected sections
    for (int y = 1; y < m_height - 1; ++y)
    {
        for (int x = 1; x < m_width - 1; ++x)
        {
            const auto& tile = m_labyrinthGrid.Get(x, y);
            if (tile == LogicalTile::Wall)
            {
                // Left-right connector case
                if (m_labyrinthGrid.Get(x - 1, y) == LogicalTile::Floor && m_labyrinthGrid.Get(x + 1, y) == LogicalTile::Floor)
                {
                    // This can technically throw, but all placed
                    // floor tiles are guaranteed to be in the map
                    int leftSection = m_tileSectionMap.at(glm::ivec2(x - 1, y));
                    int rightSection = m_tileSectionMap.at(glm::ivec2(x + 1, y));
                    if (leftSection != rightSection)
                    {
                        // Create the connector (from smaller section to larger section, by index)
                        Connector connector;
                        connector.m_connection = glm::max(leftSection, rightSection);
                        connector.m_pos = glm::ivec2(x, y);

                        // Add the connector to the smaller section
                        m_sections[glm::min(leftSection, rightSection)].m_connectors.push_back(connector);

                        // A wall can't be both a horizontal and vertical connector so it's safe to continue here
                        continue;
                    }
                }

                // Top-bottom connector case
                if (m_labyrinthGrid.Get(x, y - 1) == LogicalTile::Floor && m_labyrinthGrid.Get(x, y + 1) == LogicalTile::Floor)
                {
                    // This can technically throw, but all placed
                    // floor tiles are guaranteed to be in the map
                    int bottomSection = m_tileSectionMap.at(glm::ivec2(x, y - 1));
                    int topSection = m_tileSectionMap.at(glm::ivec2(x, y + 1));
                    if (bottomSection != topSection)
                    {
                        // Create the connector (from smaller section to larger section, by index)
                        Connector connector;
                        connector.m_connection = glm::max(bottomSection, topSection);
                        connector.m_pos = glm::ivec2(x, y);

                        // Add the connector to the smaller section
                        m_sections[glm::min(bottomSection, topSection)].m_connectors.push_back(connector);
                    }
                }
            }
        }
    }

    // Iterate all sections and knock down connectors
    for (int i = 0; i < m_sections.size(); ++i)
    {
        // Grab a reference
        auto& section = m_sections[i];

        // Keep opening up connectors until we run out
        while (section.m_connectors.size() > 0)
        {
            // Randomly select the next connector to check
            int index = m_rng.NextInt(0, section.m_connectors.size() - 1);
            auto& connector = section.m_connectors[index];

            // Grab a copy of the new section it would connect
            int newSection = connector.m_connection;

            // If the connector brings us to a yet-unconnected section
            if (!section.m_connected.contains(newSection))
            {
                // Mark the 2 sections as connected
                section.m_connected[newSection] = true;
                m_sections[newSection].m_connected[i] = true;

                // Mark all sections that were connected to either as connected to both
                for (int j = 0; j < m_sections.size(); ++j)
                {
                    auto& s = m_sections[j];
                    if (s.m_connected.contains(i))
                    {
                        s.m_connected[newSection] = true;
                        for (const auto& kvp : m_sections[newSection].m_connected)
                        {
                            s.m_connected[kvp.first] = true;
                        }
                    }
                    if (s.m_connected.contains(newSection))
                    {
                        s.m_connected[i] = true;
                        for (const auto& kvp : section.m_connected)
                        {
                            s.m_connected[kvp.first] = true;
                        }
                    }
                }
                
                // Replace the connector wall with a floor
                // TODO: Doors?
                m_labyrinthGrid.Set(connector.m_pos.x, connector.m_pos.y, LogicalTile::Floor);
            }
            
            // Delete the connector
            section.m_connectors.erase(section.m_connectors.begin() + index);
        }
    }
}

void LabyrinthManager::GenerateChunks()
{
    // Grab the current object
    auto* pObject = GetGameObject();

    // Grab a scene reference
    auto& scene = pObject->GetScene();

    // Static tile type arrays
    const int nonGoldFloors[] = {Tile::FloorSmallSquares, Tile::FloorSquare, Tile::FloorSpiral};
    const int goldFloors[] = {Tile::FloorSquareGold, Tile::FloorSpiralGold};
    const int walls[] = {Tile::WallBottom, Tile::WallBottomLeft, Tile::WallBottomRight, Tile::WallLeft, Tile::WallRight, Tile::WallTop, Tile::WallTopRight, Tile::WallTopLeft};
    const int specialWalls[] = {Tile::WallChest, Tile::WallHelmet, Tile::WallMaze, Tile::WallMinotaur, Tile::WallPillars, Tile::WallPot};

    // Tile directional lookup table
    // Maps from a bitmasked 4 bit unsigned int denoting directions of adjacent
    // walls to the specific tile ID representing best fit.
    // NOTE: Bitmask order is up, down, left, right (most-to-least significant bits).
    Tile::type wallDirID[] =
    {
        Tile::WallMaze,
        Tile::WallPillars,
        Tile::WallPillars,
        Tile::WallBottom,
        Tile::WallPillars,
        Tile::WallTopLeft,
        Tile::WallTopRight,
        Tile::WallMaze,
        Tile::WallPillars,
        Tile::WallBottomLeft,
        Tile::WallBottomRight,
        Tile::WallMaze,
        Tile::WallRight,
        Tile::WallMaze,
        Tile::WallMaze,
        Tile::WallMinotaur
    };

    Tile::type floorVarID[] =
    {
        Tile::WallMinotaur, // Should never happen! (enclosed floor is inaccessible)
        Tile::FloorSmallSquares,
        Tile::FloorSmallSquares,
        Tile::FloorSmallSquares,
        Tile::FloorSmallSquares,
        Tile::FloorSmallSquares,
        Tile::FloorSmallSquares,
        Tile::FloorSpiral,
        Tile::FloorSmallSquares,
        Tile::FloorSmallSquares,
        Tile::FloorSmallSquares,
        Tile::FloorSpiral,
        Tile::FloorSmallSquares,
        Tile::FloorSpiral,
        Tile::FloorSpiral,
        Tile::FloorSquare,
    };

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
            auto& chunkObj = scene.CreateObject();
            pObject->AddChild(chunkObj);

            // Add the chunk object to the map
            ChunkData cd;
            cd.m_pObject = &chunkObj;
            m_chunkMap[chunkID] = cd;

            // Create the tilemap object
            auto& tilemapObj = scene.CreateObject2D();
            chunkObj.AddChild(tilemapObj);

            // Set transform offset and scale
            int xoffset = cx * CHUNK_SIZE;
            int yoffset = cy *  CHUNK_SIZE;
            auto* pTransform = tilemapObj.GetComponent<wolf::Transform2D>();
            pTransform->SetPosition(glm::vec2(xoffset * TILE_SIZE * SCALE, yoffset * TILE_SIZE * SCALE));
            pTransform->SetScale(glm::vec2(SCALE));

            // Create tilemap
            auto& tilemap = tilemapObj.AddComponent<wolf::TileMap>(CHUNK_SIZE, CHUNK_SIZE);
            tilemap.LoadTileSet("data/labyrinth.tileset");

            // Create collider component
            auto& collider = tilemapObj.AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, false);

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
                    const LogicalTile& logicalTile = m_labyrinthGrid.Get(worldPos.x, worldPos.y);

                    // Convert from logical tile to specific tile ID
                    int tile = wolf::TileMap::EMPTY_TILE;
                    unsigned char up = 0;
                    unsigned char down = 0;
                    unsigned char left = 0;
                    unsigned char right = 0;
                    unsigned char mask = 0;
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
                            
                            // Choose a random non-gold floor tile
                            // tile = nonGoldFloors[m_rng.NextInt(0, sizeof(nonGoldFloors) / sizeof(int) - 1)];

                            // Grab values for adjacent perpendicular floors
                            up = worldPos.y == m_height - 1 ? 0 : m_labyrinthGrid.Get(worldPos.x, worldPos.y + 1) == LogicalTile::Floor ? 1 : 0;
                            down = worldPos.y == 0 ? 0 : m_labyrinthGrid.Get(worldPos.x, worldPos.y - 1) == LogicalTile::Floor ? 1 : 0;
                            left = worldPos.x == 0 ? 0 : m_labyrinthGrid.Get(worldPos.x - 1, worldPos.y) == LogicalTile::Floor ? 1 : 0;
                            right = worldPos.x == m_width - 1 ? 0 : m_labyrinthGrid.Get(worldPos.x + 1, worldPos.y) == LogicalTile::Floor ? 1 : 0;

                            // Combine and align into bitmasked index
                            mask = (up << 3) | (down << 2) | (left << 1) | right;

                            // Lookup tile for configuration
                            tile = floorVarID[mask];
                            break;
                        
                        case LogicalTile::Grass:
                            tile = Tile::Grass;
                            break;
                        
                        case LogicalTile::Wall:

                            // Grab values for adjacent perpendicular walls
                            up = worldPos.y == m_height - 1 ? 0 : m_labyrinthGrid.Get(worldPos.x, worldPos.y + 1) == LogicalTile::Wall ? 1 : 0;
                            down = worldPos.y == 0 ? 0 : m_labyrinthGrid.Get(worldPos.x, worldPos.y - 1) == LogicalTile::Wall ? 1 : 0;
                            left = worldPos.x == 0 ? 0 : m_labyrinthGrid.Get(worldPos.x - 1, worldPos.y) == LogicalTile::Wall ? 1 : 0;
                            right = worldPos.x == m_width - 1 ? 0 : m_labyrinthGrid.Get(worldPos.x + 1, worldPos.y) == LogicalTile::Wall ? 1 : 0;

                            // Combine and align into bitmasked index
                            mask = (up << 3) | (down << 2) | (left << 1) | right;

                            // Lookup tile for configuration
                            tile = wallDirID[mask];

                            // Must be a bottom edge tile
                            if (!down || y == 0)
                            {
                                if (up)
                                {
                                    int numAdjacent = 1;
                                    glm::ivec2 nextPos = worldPos + glm::ivec2(0, numAdjacent + 1);
                                    while (nextPos.y - yoffset < CHUNK_SIZE)
                                    {
                                        if (m_labyrinthGrid.Get(nextPos.x, nextPos.y) != LogicalTile::Wall) break;
                                        numAdjacent++;
                                        nextPos.y++;
                                    }
                                    
                                    // Add wall tile collider
                                    collider.AddColliderBox(glm::vec2(TILE_SIZE * SCALE, TILE_SIZE * SCALE * (numAdjacent + 1)),
                                                            glm::vec2(x * SCALE * TILE_SIZE, (y + numAdjacent + 1) * SCALE * TILE_SIZE));
                                }
                                else if ((!left || !right) && y == 0)
                                {
                                    collider.AddColliderBox(glm::vec2(TILE_SIZE * SCALE),
                                                            glm::vec2(x * SCALE * TILE_SIZE, (y + 1) * SCALE * TILE_SIZE));
                                }
                            }

                            // Must be a left edge tile
                            if (!left || x == 0)
                            {
                                if (right)
                                {
                                    int numAdjacent = 1;
                                    glm::ivec2 nextPos = worldPos + glm::ivec2(numAdjacent + 1, 0);
                                    while (nextPos.x - xoffset < CHUNK_SIZE)
                                    {
                                        if (m_labyrinthGrid.Get(nextPos.x, nextPos.y) != LogicalTile::Wall) break;
                                        numAdjacent++;
                                        nextPos.x++;
                                    }
                                    
                                    // Add wall tile collider
                                    collider.AddColliderBox(glm::vec2(TILE_SIZE * SCALE * (numAdjacent + 1), TILE_SIZE * SCALE),
                                                            glm::vec2(x * SCALE * TILE_SIZE, (y + 1) * SCALE * TILE_SIZE));
                                }
                                else if ((!up || !down) && x == 0)
                                {
                                    collider.AddColliderBox(glm::vec2(TILE_SIZE * SCALE),
                                                            glm::vec2(x * SCALE * TILE_SIZE, (y + 1) * SCALE * TILE_SIZE));
                                }
                            }

                            break;
                    }

                    tilemap.SetTile(x, y, tile);
                }
            }
        }
    }
}

void LabyrinthManager::PopulateEntities(const std::vector<LabyrinthManager::Room>& placedRooms)
{
    auto* pObject = GetGameObject();

    // Load enemy data
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");
    EnemyData minitaurData = loader.LoadEnemyData("minitaur");
    EnemyData harpyData = loader.LoadEnemyData("harpy");
    EnemyData gorgonData = loader.LoadEnemyData("gorgon");

    MinitaurBuilder minitaurBuilder(pObject->GetScene());
    HarpyBuilder harpyBuilder(pObject->GetScene());
    GorgonBuilder gorgonBuilder(pObject->GetScene());

    for (const auto& room : placedRooms)
    {
        // Create array of empty tiles
        std::vector<glm::ivec2> emptyTiles;
        for (int y = 0; y < room.m_bounds.m_size.y; ++y)
        {
            for (int x = 0; x < room.m_bounds.m_size.x; ++x)
            {
                emptyTiles.push_back(glm::ivec2(x + room.m_bounds.m_origin.x, y + room.m_bounds.m_origin.y));
            }
        }

        for (const auto& entity : room.m_entitySpawns)
        {
            // Calculate position for empty tile
            glm::vec2 pos(0,0);
            if (entity.m_spawnPosType == Room::SpawnPosType::Manual) pos = glm::vec2(entity.m_pos + room.m_bounds.m_origin);
            if (entity.m_spawnPosType == Room::SpawnPosType::Center)
            {
                // Calculate center tile index
                glm::ivec2 ipos = glm::vec2(room.m_bounds.m_origin.x + room.m_bounds.m_size.x / 2,
                                            room.m_bounds.m_origin.y + room.m_bounds.m_size.y / 2);
                
                // Remove center tile from empty tiles
                for (auto iter = emptyTiles.begin(); iter != emptyTiles.end(); ++iter)
                {
                    if (ipos == *iter)
                    {
                        emptyTiles.erase(iter);
                        break;
                    }
                }
                pos = glm::vec2(ipos);
            }

            // Offset and scale floating point position
            pos += glm::vec2(0.5f);
            pos *= TILE_SIZE * SCALE;

            // Iterate all instances of the entity to spawn
            for (int i = 0; i < entity.m_amount; ++i)
            {
                // Generate a new position if random is selected
                if (entity.m_spawnPosType == Room::SpawnPosType::Random)
                {
                    // Ensure there are empty tiles left
                    if (emptyTiles.size() == 0)
                    {
                        wolf::Error("Too many spawns in room: ", room.m_name, ", no empty tiles!");
                        break;
                    }

                    // Pick a random empty tile, offset and scale
                    int tile = m_rng.NextInt(0, emptyTiles.size() - 1);
                    pos = glm::vec2(emptyTiles[tile]);
                    pos += glm::vec2(0.5f);
                    pos *= TILE_SIZE * SCALE;
                    emptyTiles.erase(emptyTiles.begin() + tile);
                }

                // Build entity based on type
                switch (entity.m_type)
                {
                    case Room::EntityType::Minitaur:
                    {
                        // Build Minitaur at the given position
                        wolf::GameObject& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, pos, m_pColliderManager);

                        // Scale the minitaur
                        minitaur.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(SCALE));

                        // Add as a child object of the correct chunk
                        auto chunkID = GetChunkID(pos);
                        auto* pChunk = GetChunk(chunkID);

                        if (!pChunk)
                        {
                            // Warn if chunk doesn't exist
                            wolf::Warning("Enemy spawned in non-existant chunk, pls fix!");

                            // Fall back on adding to main labyrinth object
                            pObject->AddChild(minitaur);
                            break;
                        }
                        
                        pChunk->AddChild(minitaur);
                        break;
                    }

                    case Room::EntityType::Harpy:
                    {
                        // Build Harpy at the given position
                        wolf::GameObject& harpy = harpyBuilder.BuildHarpy(harpyData, pos, m_pColliderManager);

                        // Scale the harpy
                        harpy.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(SCALE));

                        // Add as a child object of the correct chunk
                        auto chunkID = GetChunkID(pos);
                        auto* pChunk = GetChunk(chunkID);

                        if (!pChunk)
                        {
                            // Warn if chunk doesn't exist
                            wolf::Warning("Enemy spawned in non-existant chunk, pls fix!");

                            // Fall back on adding to main labyrinth object
                            pObject->AddChild(harpy);
                            break;
                        }
                        
                        pChunk->AddChild(harpy);
                        break;
                    }

                    case Room::EntityType::Gorgon:
                    {
                        // Build Gorgon at the given position
                        wolf::GameObject& gorgon = gorgonBuilder.BuildGorgon(gorgonData, pos, m_pColliderManager);

                        // Scale the gorgon
                        gorgon.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(SCALE));

                        // Add as a child object of the correct chunk
                        auto chunkID = GetChunkID(pos);
                        auto* pChunk = GetChunk(chunkID);

                        if (!pChunk)
                        {
                            // Warn if chunk doesn't exist
                            wolf::Warning("Enemy spawned in non-existant chunk, pls fix!");

                            // Fall back on adding to main labyrinth object
                            pObject->AddChild(gorgon);
                            break;
                        }
                        
                        pChunk->AddChild(gorgon);
                        break;
                    }

                    case Room::EntityType::CommonChest:
                    case Room::EntityType::UncommonChest:
                    case Room::EntityType::RareChest:
                    case Room::EntityType::EpicChest:
                    case Room::EntityType::LegendaryChest:
                    {
                        std::string lootTablePath;
                        std::string frameName;
                        if (entity.m_type == Room::EntityType::CommonChest)
                        {
                            lootTablePath = "data/chest_loot_common.yaml";
                            frameName = "CommonClosed";
                        }
                        if (entity.m_type == Room::EntityType::UncommonChest)
                        {
                            lootTablePath = "data/chest_loot_uncommon.yaml";
                            frameName = "UncommonClosed";
                        }
                        if (entity.m_type == Room::EntityType::RareChest)
                        {
                            lootTablePath = "data/chest_loot_rare.yaml";
                            frameName = "RareClosed";
                        }
                        if (entity.m_type == Room::EntityType::EpicChest)
                        {
                            lootTablePath = "data/chest_loot_epic.yaml";
                            frameName = "EpicClosed";
                        }
                        if (entity.m_type == Room::EntityType::LegendaryChest)
                        {
                            lootTablePath = "data/chest_loot_legendary.yaml";
                            frameName = "LegendaryClosed";
                        }

                        // Create the chest object
                        auto& chest = pObject->GetScene().CreateObject2D();

                        // Scale the chest
                        auto& transform = *chest.GetComponent<wolf::Transform2D>();
                        transform.SetPosition(pos);
                        transform.SetScale(glm::vec2(SCALE));

                        // Add the sprite
                        auto& sprite = chest.AddComponent<AnimatedSprite2D>("data/chest_anim_init.yaml");
                        sprite.SetAnimation(frameName);
                        sprite.SetOriginToCenterOfFrame();
                        
                        // Add collider
                        auto& collider = chest.AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, true);
                        collider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16, 16));

                        // Add the chest inventory
                        auto& chestInv = chest.AddComponent<ChestInventoryComponent>(16, 4, ImVec2(800, 450));
                        chestInv.FillFromLootTable(lootTablePath, m_rng);

                        // Add chest as a child object of the correct chunk
                        GetChunk(GetChunkID(pos))->AddChild(chest);
                        break;
                    }

                    // !-- Aurora added this --!
                    case Room::EntityType::DaedalusDispensary:
                    {
                        // ?-- It would be nice to choose the loot table randomly or based on where the dispensary is spawned
                        //     could use an RNG to index an array or use numbered filenames e.g. "dispensary_loot_N.yaml" --?
                        std::string strLootTablePath = "data/dispensary_contents" + std::to_string(m_rng.NextInt(1, 2)) + ".yaml";

                        // Create the dispensary object
                        auto& dispensary = pObject->GetScene().CreateObject2D();

                        // Place and scale the dispensary
                        auto& transform = *dispensary.GetComponent<wolf::Transform2D>();
                        transform.SetPosition(pos);
                        transform.SetScale(glm::vec2(SCALE));

                        // Set up the animated sprite
                        auto& animSprite = dispensary.AddComponent<AnimatedSprite2D>("data/dispensary_anim_init.yaml");

                        // Add the dispensary inventory
                        auto& inventory = dispensary.AddComponent<DispensaryInventoryComponent>(16, 4, ImVec2(50, 300));
                        inventory.FillInventoryFromFile(strLootTablePath);

                        // Add the collider
                        auto& collider = dispensary.AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, true);
                        collider.AddColliderBox(glm::vec2(22.0f, 29.0f), glm::vec2(-11.0f, 16.0f));

                        // Create the icon
                        auto& icon = pObject->GetScene().CreateObject2D();
                        
                        // Set up the icon's animated sprite
                        auto& iconSprite = icon.AddComponent<AnimatedSprite2D>("data/item_icons_anim_init.yaml");
                        
                        // Add the icon as a child object of the dispensary
                        dispensary.AddChild(icon);
                        
                        // Position the child
                        auto& iconTransform = *icon.GetComponent<wolf::Transform2D>();
                        iconTransform.SetPosition(glm::vec2(0.0f, 25.0f));
                        iconTransform.SetScale(glm::vec2(0.5f, 0.5f));

                        // Add dispensary as a child object of the correct chunk
                        GetChunk(GetChunkID(pos))->AddChild(dispensary);
                        break;
                    }

                    case Room::EntityType::SpikeTrap:
                    {
                        // Create the trap object
                        auto& trap = pObject->GetScene().CreateObject2D();

                        // Add sprite
                        auto& sprite = trap.AddComponent<wolf::Sprite2D>("data/textures/SpikesRetracted.png");
                        sprite.SetOriginToCenterOfTexture();
                        sprite.SetLayer(0);

                        // Set position
                        auto& transform = *trap.GetComponent<wolf::Transform2D>();
                        transform.SetPosition(pos);
                        transform.SetScale(glm::vec2(SCALE));

                        // Add a collider for interaction
                        auto& collider = trap.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 0, 1);
                        collider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));

                        // Add the TriggerComponent
                        trap.AddComponent<TriggerComponent>(m_pColliderManager, TriggerType::REUSABLE);

                        // Add the object to the correct chunk
                        GetChunk(GetChunkID(pos))->AddChild(trap);
                        break;
                    }
                }
            }
        }
    }
}

void LabyrinthManager::GenerateEntrance()
{
    auto* pObject = GetGameObject();

    auto& spawnRoomObj = pObject->GetScene().CreateObject2D();
    pObject->AddChild(spawnRoomObj);

    // Create the initial tilemap
    auto& tilemap = spawnRoomObj.AddComponent<wolf::TileMap>(m_spawnPatchSize.x, m_spawnPatchSize.y);
    tilemap.LoadTileSet("data/labyrinth.tileset");
    tilemap.Clear(Tile::Grass);

    glm::vec2 patchOrigin = glm::vec2((m_width / 2 * TILE_SIZE - (m_spawnPatchSize.x / 2 * TILE_SIZE)) * SCALE, -m_spawnPatchSize.y * TILE_SIZE * SCALE);

    // Position and scale the object
    auto& transform = *spawnRoomObj.GetComponent<wolf::Transform2D>();
    transform.SetPosition(patchOrigin);
    transform.SetScale(glm::vec2(SCALE));

    int left = m_spawnPatchSize.x / 2 - (m_spawnRoomSize.x / 2) - 1;
    int right = m_spawnPatchSize.x / 2 + (m_spawnRoomSize.x / 2) + 1;

    // Place walls
    for (int y = m_spawnPatchSize.y - 1; y >= m_spawnPatchSize.y - m_spawnRoomSize.y; --y)
    {
        tilemap.SetTile(left, y, Tile::WallLeft);
        tilemap.SetTile(right, y, Tile::WallLeft);
    }
    for (int x = left; x <= right; ++x)
    {
        tilemap.SetTile(x, m_spawnPatchSize.y - m_spawnRoomSize.y - 1, Tile::WallBottom);
    }

    // Corners
    tilemap.SetTile(left, m_spawnPatchSize.y - m_spawnRoomSize.y - 1, Tile::WallBottomLeft);
    tilemap.SetTile(right, m_spawnPatchSize.y - m_spawnRoomSize.y - 1, Tile::WallBottomRight);

    // Place floors
    for (int y = m_spawnPatchSize.y - 1; y >= m_spawnPatchSize.y - m_spawnRoomSize.y; --y)
    {
        for (int x = m_spawnPatchSize.x / 2 - (m_spawnRoomSize.x / 2); x <= m_spawnPatchSize.x / 2 + (m_spawnRoomSize.x / 2); ++x)
        {
            tilemap.SetTile(x, y, Tile::FloorSpiral);
        }
    }

    // Place walls
    auto& collider = spawnRoomObj.AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, false);
    collider.AddColliderBox(glm::vec2(672, 96), glm::vec2(864, 1920));
    collider.AddColliderBox(glm::vec2(96, 576), glm::vec2(864, 2400));
    collider.AddColliderBox(glm::vec2(96, 576), glm::vec2(1440, 2400));

    // Place starting dispensary

    // Create the dispensary object
    auto& dispensary = pObject->GetScene().CreateObject2D();
    pObject->AddChild(dispensary);

    // Place and scale the dispensary
    auto& dispensaryTransform = *dispensary.GetComponent<wolf::Transform2D>();
    dispensaryTransform.SetPosition(GetSpawnLocation() + glm::vec2(96, 0));
    dispensaryTransform.SetScale(glm::vec2(SCALE));

    // Set up the animated sprite
    auto& animSprite = dispensary.AddComponent<AnimatedSprite2D>("data/dispensary_anim_init.yaml");

    // Add the dispensary inventory
    auto& inventory = dispensary.AddComponent<DispensaryInventoryComponent>(16, 4, ImVec2(50, 300));
    inventory.FillInventoryFromFile("data/dispensary_contents1.yaml");

    // Add the collider
    auto& dispensaryCollider = dispensary.AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, true);
    dispensaryCollider.AddColliderBox(glm::vec2(22.0f, 29.0f), glm::vec2(-11.0f, 16.0f));

    // Create the icon
    auto& icon = pObject->GetScene().CreateObject2D();
    
    // Set up the icon's animated sprite
    auto& iconSprite = icon.AddComponent<AnimatedSprite2D>("data/item_icons_anim_init.yaml");
    
    // Add the icon as a child object of the dispensary
    dispensary.AddChild(icon);
    
    // Position the child
    auto& iconTransform = *icon.GetComponent<wolf::Transform2D>();
    iconTransform.SetPosition(glm::vec2(0.0f, 25.0f));
    iconTransform.SetScale(glm::vec2(0.5f, 0.5f));
}