#include "DialogueAndCutsceneState.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "W_TextureManager.h"
#include "W_Input.h"
#include <imgui/imgui.h>

// Utility function to add two ImVec2
ImVec2 addImVec2(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}

DialogueAndCutsceneState::DialogueAndCutsceneState(GameStateManager* manager, Theseus* gameInstance, const std::string& yamlFilePath)
    : GameState(manager, gameInstance), m_yamlFilePath(yamlFilePath) {}


void DialogueAndCutsceneState::Enter() {
    try {
        LoadFromYAML(m_yamlFilePath);
        m_mode = Mode::None; // Start in a neutral mode
        std::cout << "DialogueAndCutsceneState: YAML loaded successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading YAML: " << e.what() << std::endl;
        // Handle the error or set fallback values
        m_mode = Mode::None;
    }
}



void DialogueAndCutsceneState::Exit() {
    m_mode = Mode::None;
    m_currentDialogueLines.clear();
    m_currentCutsceneKeyframes.clear();
    m_activeDialogueID.clear();
    // Clean up textures
    for (auto& dialoguePair : m_dialogues) {
        for (auto& character : dialoguePair.second.characters) {
            if (character.portraitTexture) {
                wolf::TextureManager::DestroyTexture(character.portraitTexture);
            }
        }
    }
}

void DialogueAndCutsceneState::Update(float delta) {
    if (m_mode == Mode::Dialogue) {
        UpdateDialogue(delta);
    } else if (m_mode == Mode::Cutscene) {
        AdvanceCutscene(delta);
    }
}
void DialogueAndCutsceneState::Render() {
    if (m_mode == Mode::Dialogue) {
        RenderDialogue();
    }
}

void DialogueAndCutsceneState::UpdateDialogue(float delta) {
    if (!m_isDialogueActive) return;

    m_timeSinceLastKeyframe += delta;
    const std::string& currentLine = GetCurrentDialogueLine();
    m_isLineFinished = m_timeSinceLastKeyframe >= currentLine.length() * 0.05f || m_showFullText;

    bool isInputPressed = wolf::Input::IsLMBJustDown() || wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE);
    bool isAnyButtonHovered = ImGui::IsAnyItemHovered();
    if (isInputPressed && !isAnyButtonHovered) {
        if (!m_showFullText && !m_isLineFinished) {
            m_showFullText = true;
        } else if (m_isLineFinished && m_currentLineIndex < m_currentDialogueLines.size() - 1) {
            AdvanceDialogue();
        }
    }

    if (m_autoplay && m_timeSinceLastKeyframe > m_autoPlayDelay && m_isLineFinished) {
        AdvanceDialogue();
    }
}

void DialogueAndCutsceneState::RenderDialogue() {
    if (!m_isDialogueActive) return;  // Skip rendering if dialogue is inactive

    // Smooth fade-in effect for the dialogue box
    static float fadeOpacity = 0.0f;
    fadeOpacity = std::min(fadeOpacity + 0.05f, 1.0f);  // Gradually increase opacity

    // Check if we're on the last line of the dialogue
    const auto& lines = m_dialogues[m_activeDialogueID].lines;
    bool isLastLine = (m_currentLineIndex >= lines.size() - 1);

    // Screen dimensions
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float dialogueWidth = screenSize.x * 0.75f;  // Set width to 75% of screen
    float baseHeight = 100.0f;  // Base height for window padding and controls

    // Calculate text height
    const std::string& currentLine = GetCurrentDialogueLine();
    float textHeight = ImGui::CalcTextSize(currentLine.c_str(), nullptr, true, dialogueWidth - 100).y;

    // Add extra height if there's a portrait
    bool hasPortrait = false;
    float portraitHeight = 0.0f;
    wolf::Texture* portraitTexture = nullptr;
    if (!m_activeDialogueID.empty()) {
        const std::string& characterName = GetCurrentCharacterName();
        for (auto& character : m_dialogues[m_activeDialogueID].characters) {
            if (character.name == characterName) {
                portraitTexture = character.portraitTexture; // Retrieve the portrait texture
                hasPortrait = (portraitTexture != nullptr);
                portraitHeight = 64.0f;  // Set portrait height (adjust as needed)
                break;
            }
        }
    }

    // Calculate total window height
    float windowHeight = baseHeight + std::max(textHeight, portraitHeight) + 80.0f;  // Add padding for buttons and controls

    // Calculate window position
    ImVec2 windowSize = ImVec2(dialogueWidth, windowHeight);
    ImVec2 windowPos = ImVec2((screenSize.x - windowSize.x) / 2, screenSize.y - windowSize.y - 50);

    // Enhanced Layered Shadow Effect
    auto* drawList = ImGui::GetBackgroundDrawList();
    for (int i = 0; i < 5; i++) {
        float shadowOpacity = 0.05f * (i + 1) * fadeOpacity;
        float shadowOffset = 10.0f + (i * 2.0f);
        drawList->AddRectFilled(
            addImVec2(windowPos, ImVec2(-shadowOffset, -shadowOffset)),
            addImVec2(addImVec2(windowPos, windowSize), ImVec2(shadowOffset, shadowOffset)),
            ImColor(0.0f, 0.0f, 0.0f, shadowOpacity),
            20.0f
        );
    }

    // Gradient Background
    drawList->AddRectFilledMultiColor(
        windowPos,
        addImVec2(windowPos, windowSize),
        ImColor(30, 34, 42, static_cast<int>(fadeOpacity * 255)),
        ImColor(30, 34, 42, static_cast<int>(fadeOpacity * 255)),
        ImColor(24, 26, 32, static_cast<int>(fadeOpacity * 255)),
        ImColor(24, 26, 32, static_cast<int>(fadeOpacity * 255))
    );

    // ImGui window styling
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 25));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 15));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.5f, 0.7f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.95f, fadeOpacity));

    ImGui::Begin("EnhancedDialogue", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Display character name and portrait if available
    if (!m_activeDialogueID.empty()) 
    {
        const std::string& characterName = GetCurrentCharacterName();
        if (hasPortrait && portraitTexture)
        {
            // Display the character portrait
            ImGui::Image((void*)(intptr_t)portraitTexture->GetID(), ImVec2(64, 64));
            ImGui::SameLine();
        }
        ImGui::TextColored(ImVec4(0.8f, 0.85f, 1.0f, 1.0f), "%s:", characterName.c_str());

        // Accent line above text
        ImVec2 start = addImVec2(ImGui::GetCursorScreenPos(), ImVec2(0, -10));
        ImVec2 end = addImVec2(ImGui::GetCursorScreenPos(), ImVec2(windowSize.x * 0.25f, -10));
        drawList->AddLine(start, end, ImColor(0.5f, 0.7f, 1.0f, 0.6f), 2.0f);

        // Display dialogue text
        static float lineFade = 0.0f;
        lineFade = std::min(lineFade + 0.02f, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, lineFade * fadeOpacity);

        if (m_showFullText)
        {
            ImGui::TextWrapped("%s", currentLine.c_str());
        }
        else
        {
            std::string partialText = currentLine.substr(0, static_cast<int>(m_timeSinceLastKeyframe / 0.05f));
            partialText += "|";
            ImGui::TextWrapped("%s", partialText.c_str());
        }
        ImGui::PopStyleVar();

        ImGui::Dummy(ImVec2(0.0f, 20.0f));  // Whitespace below text
    }

    // Centered button layout
    bool showContinueButton = !isLastLine || !m_isLineFinished;
    float buttonWidth = showContinueButton ? 130.0f : 180.0f;  // Increase width when only two buttons are shown
    ImVec2 buttonSize(buttonWidth, 35);

    float totalButtonWidth = showContinueButton ? 420.0f : 2 * buttonWidth;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalButtonWidth) / 2);  // Center buttons

    // Button styling for a polished look
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.4f, 0.45f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.3f, 0.35f, fadeOpacity));

    // Autoplay button
    if (ImGui::Button(m_autoplay ? "Autoplay: ON" : "Autoplay: OFF", buttonSize))
    {
        m_autoplay = !m_autoplay;
    }

    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    // Continue button (if not last line or line is not finished)
    if (showContinueButton)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.45f, 0.5f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.55f, 0.6f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));

        if (ImGui::Button("Continue", buttonSize))
        {
            OnContinueButtonPressed();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
    }

    // Exit button (always visible)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.4f, 0.45f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.3f, 0.35f, fadeOpacity));

    if (ImGui::Button("Exit", buttonSize))
    {
        EndDialogue();
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);
}

void DialogueAndCutsceneState::OnContinueButtonPressed() {
    if (!m_showFullText && !m_isLineFinished) {
        m_showFullText = true;
    } else {
        AdvanceDialogue();
    }
}


void DialogueAndCutsceneState::StartDialogue(const std::string& dialogueID) {
    auto it = m_dialogues.find(dialogueID);
    if (it != m_dialogues.end()) {
        m_activeDialogueID = dialogueID;
        m_currentDialogueLines = it->second.lines;
        m_currentLineIndex = 0;
        m_isDialogueActive = true;
        m_showFullText = false;
        m_timeSinceLastKeyframe = 0.0f;

        // Set the mode to dialogue
        m_mode = Mode::Dialogue;
    } else {
        std::cerr << "Dialogue ID " << dialogueID << " not found!" << std::endl;
    }
}

void DialogueAndCutsceneState::AdvanceDialogue() {
    if (m_currentLineIndex < m_currentDialogueLines.size() - 1) {
        m_currentLineIndex++;
        const auto& currentLine = m_currentDialogueLines[m_currentLineIndex];

        if (currentLine.action == "cutscene" && !currentLine.cutsceneID.empty()) {
            // Start the cutscene and pause the dialogue
            StartCutscene(currentLine.cutsceneID);
            m_isDialogueActive = false;
        }
    } else {
        EndDialogue(); // End dialogue when all lines are processed
    }
}

void DialogueAndCutsceneState::EndDialogue() {
    m_isDialogueActive = false;
    m_activeDialogueID.clear();
    m_currentLineIndex = 0;
    m_showFullText = false;
    m_timeSinceLastKeyframe = 0.0f;

    std::cout << "Dialogue ended." << std::endl;

    // If no cutscene follows and no further dialogues, pop the state
    if (IsComplete()) {
        std::cout << "All dialogues and cutscenes are complete. Exiting state." << std::endl;
        m_pStateManager->PopState();
    }
}


std::string DialogueAndCutsceneState::GetCurrentDialogueLine() const {
    if (m_currentLineIndex < m_currentDialogueLines.size()) {
        return m_currentDialogueLines[m_currentLineIndex].text;
    }
    return "";
}

std::string DialogueAndCutsceneState::GetCurrentCharacterName() const {
    if (m_currentLineIndex < m_currentDialogueLines.size()) {
        return m_currentDialogueLines[m_currentLineIndex].characterName;
    }
    return "";
}

void DialogueAndCutsceneState::LoadFromYAML(const std::string& yamlFilePath) {
    YAML::Node root = YAML::LoadFile(yamlFilePath);

    // Parse dialogues
    if (root["dialogue"]) {
        for (const auto& dialogueNode : root["dialogue"]) {
            DialogueData dialogueData;
            std::string id = dialogueNode["id"].as<std::string>();

            // Parse characters
            if (dialogueNode["characters"]) {
                for (const auto& characterNode : dialogueNode["characters"]) {
                    CharacterData character;
                    character.name = characterNode["name"].as<std::string>();
                    character.portraitPath = characterNode["portraitPath"].as<std::string>();
                    character.portraitTexture = wolf::TextureManager::CreateTexture(character.portraitPath);
                    dialogueData.characters.push_back(character);
                }
            }

            // Parse lines
            if (dialogueNode["lines"]) {
                for (const auto& lineNode : dialogueNode["lines"]) {
                    DialogueLine line;
                    if (lineNode["character"]) {
                        line.characterName = lineNode["character"].as<std::string>();
                    }
                    if (lineNode["text"]) {
                        line.text = lineNode["text"].as<std::string>();
                    }
                    if (lineNode["duration"]) {
                        line.duration = lineNode["duration"].as<float>();
                    }
                    if (lineNode["action"]) {
                        line.action = lineNode["action"].as<std::string>();
                    }
                    if (lineNode["cutsceneID"]) {
                        line.cutsceneID = lineNode["cutsceneID"].as<std::string>();
                    }
                    dialogueData.lines.push_back(line);
                }
            }

            m_dialogues[id] = dialogueData;
        }
    }

    // Parse cutscenes
    if (root["cutscenes"]) {
        for (const auto& cutsceneEntry : root["cutscenes"]) {
            std::string cutsceneID = cutsceneEntry.first.as<std::string>();
            std::vector<CameraKeyframe> keyframes;

            for (const auto& frame : cutsceneEntry.second["camera_keyframes"]) {
                CameraKeyframe keyframe;

                if (frame["target"]) {
                    keyframe.target = frame["target"].as<std::string>();
                    if (m_pGameInstance->GetSharedContext().HasEntity(keyframe.target)) {
                        auto entityID = m_pGameInstance->GetSharedContext().GetEntityID(keyframe.target);
                        auto targetObject = m_pGameInstance->GetScene().GetObject(entityID);
                        if (targetObject) {
                            auto transform = targetObject->GetComponent<wolf::Transform2D>();
                            if (transform) {
                                keyframe.position = transform->GetGlobalPosition();
                            }
                        }
                    } else {
                        std::cerr << "Warning: Target entity " << keyframe.target << " not found in SharedContext!" << std::endl;
                    }
                }

                if (frame["zoom"]) {
                    keyframe.zoom = frame["zoom"].as<float>();
                } else {
                    keyframe.zoom = 1.0f; // Default zoom level
                }
                if (frame["duration"]) {
                    keyframe.duration = frame["duration"].as<float>();
                } else {
                    keyframe.duration = 1.0f; // Default duration
                }
                keyframes.push_back(keyframe);
            }

            m_cutscenes[cutsceneID] = keyframes;
        }
    }
}

void DialogueAndCutsceneState::StartCutscene(const std::string& cutsceneID) {
    auto it = m_cutscenes.find(cutsceneID);
    if (it == m_cutscenes.end()) {
        std::cerr << "Cutscene ID " << cutsceneID << " not found!" << std::endl;
        return;
    }

    m_currentCutsceneKeyframes = it->second;
    m_currentKeyframeIndex = 0;
    m_cutsceneTimer = 0.0f;
    m_mode = Mode::Cutscene;

    if (!m_currentCutsceneKeyframes.empty()) {
        const CameraKeyframe& firstKeyframe = m_currentCutsceneKeyframes[0];

        if (m_pGameInstance->GetSharedContext().HasEntity(firstKeyframe.target)) {
            auto entityID = m_pGameInstance->GetSharedContext().GetEntityID(firstKeyframe.target);
            auto targetObject = m_pGameInstance->GetScene().GetObject(entityID);
            if (targetObject) {
                auto transform = targetObject->GetComponent<wolf::Transform2D>();
                if (transform) {
                    m_currentCameraPosition = transform->GetGlobalPosition();
                }
            }
        } else {
            m_currentCameraPosition = firstKeyframe.position;  // Use predefined position if target not found
        }
        m_currentZoomLevel = firstKeyframe.zoom;
    }

    std::cout << "Cutscene " << cutsceneID << " started." << std::endl;
}
void DialogueAndCutsceneState::AdvanceCutscene(float delta) {
    if (m_currentKeyframeIndex >= m_currentCutsceneKeyframes.size()) {
        EndCutscene();  // End the cutscene when all keyframes are processed
        return;
    }

    // Get the active camera
    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (!camera) {
        std::cerr << "Error: No active camera found!" << std::endl;
        EndCutscene();
        return;
    }

    const CameraKeyframe& currentKeyframe = m_currentCutsceneKeyframes[m_currentKeyframeIndex];
    m_cutsceneTimer += delta;

    // Interpolation logic
    if (m_currentKeyframeIndex < m_currentCutsceneKeyframes.size() - 1) {
        const CameraKeyframe& nextKeyframe = m_currentCutsceneKeyframes[m_currentKeyframeIndex + 1];

        // Calculate interpolation factor
        float t = glm::clamp(m_cutsceneTimer / currentKeyframe.duration, 0.0f, 1.0f);

        // Smoothly interpolate position and zoom
        m_currentCameraPosition = glm::mix(currentKeyframe.position, nextKeyframe.position, t);
        m_currentZoomLevel = glm::mix(currentKeyframe.zoom, nextKeyframe.zoom, t);

        // Apply transformations to the camera
        camera->SetPosition(m_currentCameraPosition);
        camera->SetZoom(m_currentZoomLevel);

        // Print current zoom value

        // Advance to the next keyframe
        if (t >= 1.0f) {
            m_cutsceneTimer = 0.0f;
            m_currentKeyframeIndex++;
        }
    } else {
        // Apply final keyframe position and zoom
        m_currentCameraPosition = currentKeyframe.position;
        m_currentZoomLevel = currentKeyframe.zoom;
        camera->SetPosition(m_currentCameraPosition);
        camera->SetZoom(m_currentZoomLevel);

        // Print current zoom value

        // End the cutscene if this is the last keyframe
        EndCutscene();
    }
}

void DialogueAndCutsceneState::EndCutscene() {
    m_mode = Mode::None;
    m_currentCutsceneKeyframes.clear();
    m_currentKeyframeIndex = 0;
    m_cutsceneTimer = 0.0f;

    std::cout << "Cutscene ended." << std::endl;

    // Resume dialogue if applicable
    if (!m_activeDialogueID.empty() && m_currentLineIndex < m_currentDialogueLines.size()) {
        m_isDialogueActive = true;
        m_mode = Mode::Dialogue;
    } else if (IsComplete()) {
        // No more dialogues or cutscenes, pop the state
        std::cout << "All dialogues and cutscenes are complete. Exiting state." << std::endl;
        m_pStateManager->PopState();
    }
}


void DialogueAndCutsceneState::Pause() {
    // Handle any required logic when pausing the state
    std::cout << "DialogueAndCutsceneState paused." << std::endl;
}

void DialogueAndCutsceneState::Resume() {
    // Handle any required logic when resuming the state
    std::cout << "DialogueAndCutsceneState resumed." << std::endl;
}

bool DialogueAndCutsceneState::IsComplete() const {
    return m_activeDialogueID.empty() && m_currentCutsceneKeyframes.empty();
}