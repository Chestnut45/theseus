#include "LabyrinthBuilder.h"

// ImGui for GUI windows
#include <imgui/imgui.h>

// Icon font definitions
#include <IconsFontAwesome6.h>

#include <W_Logging.h>
#include <W_TileMap.h>
#include <W_Transform2D.h>

LabyrinthBuilder::LabyrinthBuilder()
{
}

LabyrinthBuilder::~LabyrinthBuilder()
{
}

void LabyrinthBuilder::GenerateLabyrinth()
{
    // Only bother if valid generation parameters
    if (m_width <= 0 || m_height <= 0)
    {
        wolf::Error("Can't generate labyrinth, invalid width / height");
        return;
    }

    auto* object = GetGameObject();
    if (object)
    {
        // Set the rng seed
        m_RNG.SetSeed(m_seed);

        // Grab a scene reference
        auto& scene = object->GetScene();

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
}

void LabyrinthBuilder::DestroyLabyrinth()
{
    auto* object = GetGameObject();
    if (object)
    {
        object->DeleteAllChildren();
    }
}

void LabyrinthBuilder::Regenerate()
{
    DestroyLabyrinth();
    GenerateLabyrinth();
}

void LabyrinthBuilder::ShowGUI()
{
    // Setup window flags
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar;

    // Set window position and size
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({256, 256});
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
    if (ImGui::Button("Randomize Seed")) m_seed = m_RNG.NextInt(0, INT32_MAX);
    ImGui::InputInt("Seed", &m_seed);
    ImGui::InputInt("Width", &m_width);
    ImGui::InputInt("Height", &m_height);

    ImGui::SeparatorText("Controls");

    // Buttons to destroy / regenerate the labyrinth
    if (ImGui::Button("Destroy")) DestroyLabyrinth();
    ImGui::SameLine();
    if (ImGui::Button("Regenerate")) Regenerate();

    // End of window
    ImGui::End();
}