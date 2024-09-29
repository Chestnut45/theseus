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

    // Grab a scene reference
    auto& scene = object->GetScene();

    // Reseed the rng before generating
    if (m_randomizeSeed) m_RNG.SetSeed(m_RNG.NextInt(0, INT32_MAX));
    else m_RNG.Reseed();

    // TODO: Place all rooms into the labyrinth data structure

    // TODO: Generate maze paths between all rooms

    // TODO: Determine chunks to generate

    // TODO: Generate chunks one-by-one (tilemap, entity spawns, etc.)

    // Add a test tilemap as a child object
    auto& tileMapObject = scene.CreateObject2D();
    object->AddChild(tileMapObject);

    // Add the tilemap component
    auto& tileMap = tileMapObject.AddComponent<wolf::TileMap>(m_width, m_height);
    tileMapObject.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));
    tileMap.LoadTileSet("data/labyrinth.tileset");
    
    // Quick test of procedural generation
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            // Place walls around the edge
            if (x == 0 || x == m_width - 1 || y == 0 || y == m_height - 1)
            {
                // Except for the entrance
                if (x == 1 && y == 0)
                {
                    tileMap.SetTile(x, y, Tile::FloorSpiralGold);
                    continue;
                }
                tileMap.SetTile(x, y, Tile::WallMaze);
            }
            else
            {
                tileMap.SetTile(x, y, m_RNG.FlipCoin() ? Tile::FloorSmallSquares : Tile::FloorSpiral);
            }
        }
    }
}

void LabyrinthManager::DestroyLabyrinth()
{
    auto* object = GetGameObject();
    if (object)
    {
        object->DeleteAllChildren();
    }
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
        int seed = (int)m_RNG.GetSeed();
        int prevSeed = seed;
        ImGui::InputInt("Seed", &seed);
        if (prevSeed != seed) m_RNG.SetSeed(seed);
    }
    ImGui::InputInt("Width", &m_width);
    ImGui::InputInt("Height", &m_height);

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