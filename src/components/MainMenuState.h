//-----------------------------------------------------------------------------
// File:			MainMenuState.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible for the Concrete MainMenu State.
//-----------------------------------------------------------------------------

#pragma once
#include "GameState.h"
#include "GameStateManager.h"
#include "PlayState.h"

class MainMenuState : public GameState
{
public:
    MainMenuState(GameStateManager* manager) : GameState(manager) {}

 void Update(float delta) override
    {
        // TODO: Implement main menu logic here
        
        // Example: Check for input to transition to the PlayState.
        // we could check if the user has pressed the "Play" button or made another selection.
        // For example, this can be linked our ui framework like ImGui
        // if (/* check if the user presses the Play button or selects 'Start Game' */) 
        // {
        //     // Transition to the PlayState when the player selects "Play"
        //     m_manager->SetState(new PlayState(m_manager)); 
        // }

        // Additional checks for other main menu options (e.g., Options, Quit)
        // if (/* user selects options */) { m_manager->SetState(new OptionsState(m_manager)); }
    }

    // Render the MainMenuState
    void Render() override
    {
        // TODO: Implement main menu rendering here
        
        // Example: Render the main menu UI elements like "Play", "Options", "Quit"
        // we can use a GUI library like ImGui or a custom solution to draw the menu
        // Example:
        //  - Display "Play" button
        //  - Display "Options" button
        //  - Display "Quit" button

        // Example with pseudocode:
        // if (ImGui::Button("Play")) { /* Handle Play button click */ }
        // if (ImGui::Button("Options")) { /* Handle Options button click */ }
        // if (ImGui::Button("Quit")) { /* Handle Quit button click */ }
    }
};