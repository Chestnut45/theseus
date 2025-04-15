//-----------------------------------------------------------------------------
// File:			DialogueAndCutsceneState.h
// Original Author:	Youssef Ashraf
// ver 1.1
// A class that's responsible for the Concrete Dialogue and Cutscene State.
//-----------------------------------------------------------------------------
#include "DialogueAndCutsceneState.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "W_TextureManager.h"
#include "W_Input.h"
#include "../events/DialogueOrCutsceneEndEvent.h"

// Constructor
DialogueAndCutsceneState::DialogueAndCutsceneState(GameStateManager* manager, Theseus* gameInstance, const std::string& yamlFilePath)
    : GameState(manager, gameInstance), m_yamlFilePath(yamlFilePath), m_triggerNPCID(-1),
      m_fadeAlpha(0.0f), m_fadeTimer(0.0f), m_fadeDuration(0.0f), m_fadingIn(false) {}

DialogueAndCutsceneState::DialogueAndCutsceneState(GameStateManager* manager, Theseus* gameInstance, const std::string& yamlFilePath, int npcID)
    : GameState(manager, gameInstance), m_yamlFilePath(yamlFilePath), m_triggerNPCID(npcID),
      m_fadeAlpha(0.0f), m_fadeTimer(0.0f), m_fadeDuration(0.0f), m_fadingIn(false) {}

// Enter
void DialogueAndCutsceneState::Enter() {
    if (!m_isYAMLLoaded) {
        try {
            LoadFromYAML(m_yamlFilePath);
            m_isYAMLLoaded = true; // Mark as loaded
            // std::cout << "DialogueAndCutsceneState: YAML loaded successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error loading YAML: " << e.what() << std::endl;
        }
    } else {
        // std::cout << "DialogueAndCutsceneState: YAML already loaded, skipping reload." << std::endl;
    }
}

// Exit
void DialogueAndCutsceneState::Exit() {
    // Send off an event to let anyone interested know that the dialogue has finished
    // Ensure safe index!
    int index = glm::clamp(static_cast<int>(m_currentSequenceIndex) - 1, 0, static_cast<int>(m_currentSequence->size()));
    wolf::EventManager::TriggerEvent(DialogueOrCutsceneEndEvent(m_currentSequence->at(index).sequenceID, m_triggerNPCID));

    // Destroy all character portraits
    for (auto& pair : m_characterPortraits) {
        wolf::TextureManager::DestroyTexture(pair.second);
    }
    m_characterPortraits.clear();

    // Clear all sequences
    m_sequences.clear();

    // Reset the current sequence index
    m_currentSequence = nullptr;
    m_currentSequenceIndex = 0;
    m_triggerNPCID = -1;
}

// Pause
void DialogueAndCutsceneState::Pause() {
    // std::cout << "DialogueAndCutsceneState paused." << std::endl;
}

// Resume
void DialogueAndCutsceneState::Resume() {
    // std::cout << "DialogueAndCutsceneState resumed." << std::endl;
}

// Update
void DialogueAndCutsceneState::Update(float delta) {
     if (m_lmbCooldown > 0.0f) {
        m_lmbCooldown -= delta; // Reduce the cooldown timer
    }
    if (m_currentSequence && m_currentSequenceIndex < m_currentSequence->size()) {
        AdvanceSequence(delta);
    } else {
        // Ensure PopState() is called if sequences are complete
        m_pStateManager->PopState();
    }
}

void DialogueAndCutsceneState::LoadSequence(const std::string& sequenceID) {
    // Step 1: Enter the state (initialize resources)
    Enter();

    // Step 2: Check if the sequence exists
    auto it = m_sequences.find(sequenceID);
    if (it == m_sequences.end()) {
        std::cerr << "DialogueAndCutsceneState: Sequence '" << sequenceID << "' not found." << std::endl;
        return;
    }

    // Step 3: Start the desired sequence
    StartSequence(sequenceID);
}

void DialogueAndCutsceneState::StartSequence(const std::string& sequenceID) {
    // Find the sequence by its ID in the map
    auto it = m_sequences.find(sequenceID);
    if (it != m_sequences.end()) {
        // Set the current sequence and reset the index
        m_currentSequence = &it->second;
        m_currentSequenceIndex = 0;

        // Reset dialogue and cutscene state variables
        m_timeSinceLastKeyframe = 0.0f;
        m_cutsceneTimer = 0.0f;
        m_currentKeyframeIndex = 0;
        m_fadeTimer = 0.0f;

        // std::cout << "Starting sequence: " << sequenceID << std::endl;
    } else {
        // Handle the case where the sequence ID is not found
        std::cerr << "Sequence ID '" << sequenceID << "' not found!" << std::endl;
    }
}

void DialogueAndCutsceneState::AdvanceSequence(float delta) {
    // Ensure there is a current sequence and it's within bounds
    if (!m_currentSequence || m_currentSequenceIndex >= m_currentSequence->size()) {
        // Sequence is finished; exit the state
        m_pStateManager->PopState();
        return;
    }

    auto& currentItem = (*m_currentSequence)[m_currentSequenceIndex];

    // Track the progress of dialogue, cutscene, and fade
    bool dialogueFinished = false;
    bool cutsceneFinished = false;
    bool fadeFinished = true;

    // Handle fade transitions
    if (currentItem.type == "fade") {

        // Fix for freezing causing bad fades
        if (delta > 0.25f) delta = 0.0167f;
        
        m_fadeTimer += delta;

        // Update fade alpha based on fade direction
        if (currentItem.fadeType == "to") {
            m_fadeAlpha = glm::clamp(m_fadeTimer / currentItem.fadeDuration, 0.0f, 1.0f);
        } else if (currentItem.fadeType == "from") {
            m_fadeAlpha = glm::clamp(1.0f - (m_fadeTimer / currentItem.fadeDuration), 0.0f, 1.0f);
        }

        // Check if fade transition is complete
        if (m_fadeTimer >= currentItem.fadeDuration) {
            m_fadeAlpha = (currentItem.fadeType == "to") ? 1.0f : 0.0f;
            m_fadeTimer = 0.0f;
            m_currentSequenceIndex++;
            ResetCutsceneState();
            return;
        }

        fadeFinished = false;
    }

    // Handle dialogue progression
    if ((currentItem.type == "dialogue" || currentItem.type == "combined") && fadeFinished) {
        m_timeSinceLastKeyframe += delta;

        const std::string& currentLine = GetCurrentDialogueLine();
        m_isLineFinished = (m_timeSinceLastKeyframe >= currentLine.length() * 0.05f || m_showFullText);

        // Check user input for skipping
        bool isInputPressed = (wolf::Input::IsLMBJustDown() || wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE));
        bool isLastLine = (m_currentSequenceIndex >= m_currentSequence->size() - 1);

        if (isInputPressed) {
            if (!m_showFullText) {
                m_showFullText = true;
                m_isLineFinished = true;
            } else if (!m_isLineFinished) {
                m_isLineFinished = true;
            } else if (isLastLine) {
                m_isDialogueActive = false;
            } else {
                m_timeSinceLastKeyframe = 0.0f;
                m_showFullText = false;
                m_isLineFinished = false;
                m_currentSequenceIndex++;
                ResetCutsceneState();
            }
        }

        // Handle autoplay progression if enabled
        if (m_autoplay && m_isLineFinished && m_timeSinceLastKeyframe > m_autoPlayDelay) {
            dialogueFinished = true;
        }
    }

    // Handle cutscene progression
    if ((currentItem.type == "cutscene" || currentItem.type == "combined") && fadeFinished) {
        if (m_currentKeyframeIndex >= currentItem.cutscene.size()) {
            cutsceneFinished = true;
        } else {
            const auto& targetKeyframe = currentItem.cutscene[m_currentKeyframeIndex];
            m_cutsceneTimer += delta;

            float t = glm::clamp(m_cutsceneTimer / targetKeyframe.duration, 0.0f, 1.0f);

            if (m_currentKeyframeIndex > 0) {
                const auto& previousKeyframe = currentItem.cutscene[m_currentKeyframeIndex - 1];
                m_currentCameraPosition = glm::mix(previousKeyframe.position, targetKeyframe.position, t);
                m_currentZoomLevel = glm::mix(previousKeyframe.zoom, targetKeyframe.zoom, t);
            } else {
                auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
                if (camera) {
                    glm::vec2 initialPosition = camera->GetPosition();
                    float initialZoom = camera->GetZoom();

                    m_currentCameraPosition = glm::mix(initialPosition, targetKeyframe.position, t);
                    m_currentZoomLevel = glm::mix(initialZoom, targetKeyframe.zoom, t);
                }
            }

            auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
            if (camera) {
                camera->SetPosition(m_currentCameraPosition);
                camera->SetZoom(m_currentZoomLevel);
            }

            if (t >= 1.0f) {
                m_cutsceneTimer = 0.0f;
                m_currentKeyframeIndex++;
            }

            cutsceneFinished = (m_currentKeyframeIndex >= currentItem.cutscene.size());
        }
    }

    // Combined logic
    if (currentItem.type == "combined") {
        if (cutsceneFinished && dialogueFinished && m_showFullText) {
            m_timeSinceLastKeyframe = 0.0f;
            m_showFullText = false;
            m_currentSequenceIndex++;
            ResetCutsceneState();
        }
    } else if ((currentItem.type == "dialogue" && dialogueFinished && m_showFullText) ||
               (currentItem.type == "cutscene" && cutsceneFinished)) {
        m_timeSinceLastKeyframe = 0.0f;
        m_showFullText = false;
        m_currentSequenceIndex++;
        ResetCutsceneState();
    }
}


void DialogueAndCutsceneState::ResetCutsceneState() {
    if (m_currentSequence && m_currentSequenceIndex < m_currentSequence->size()) {
        auto& nextItem = (*m_currentSequence)[m_currentSequenceIndex];


        // Reset state only for sequences involving cutscenes
        if (nextItem.type == "cutscene" || nextItem.type == "combined") {
            m_cutsceneTimer = 0.0f;
            m_currentKeyframeIndex = 0;
        }
    }
}

// YAML Parsing
void DialogueAndCutsceneState::LoadFromYAML(const std::string& yamlFilePath) {
    YAML::Node root = YAML::LoadFile(yamlFilePath);

    if (root["CutscenesAndDialogues"]) {
        for (const auto& sequenceEntry : root["CutscenesAndDialogues"]) {
            std::string sequenceID = sequenceEntry.first.as<std::string>();
            std::vector<DialogueAndCutsceneItem> sequenceItems; // Vector to hold items for this sequence

            // Parse characters
            if (sequenceEntry.second["characters"]) {
                for (const auto& characterNode : sequenceEntry.second["characters"]) {
                    CharacterData character;
                    character.name = characterNode["name"].as<std::string>();
                    character.portraitPath = characterNode["portraitPath"].as<std::string>();

                    // Load portraits into a global map (if not already loaded)
                    if (m_characterPortraits.find(character.name) == m_characterPortraits.end()) {
                        character.portraitTexture = wolf::TextureManager::CreateTexture(character.portraitPath);
                        character.portraitTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest, wolf::Texture::FilterMode::FM_Nearest);
                        m_characterPortraits[character.name] = character.portraitTexture;
                    }
                }
            }

            // Parse the sequence list
            if (sequenceEntry.second["sequence"]) {
                for (const auto& seqNode : sequenceEntry.second["sequence"]) {
                    DialogueAndCutsceneItem item;
                    item.sequenceID = sequenceID; // Assign the sequence ID
                    item.type = seqNode["type"].as<std::string>();

                    if (item.type == "dialogue") {
                        DialogueLine line;
                        line.characterName = seqNode["character"].as<std::string>();
                        line.text = seqNode["text"].as<std::string>();
                        line.duration = seqNode["duration"].as<float>();
                        item.dialogue = line;
                    } 
                    else if (item.type == "cutscene") {
                        auto& sharedContext = m_pGameInstance->GetSharedContext();

                        // Parse cutscene keyframes
                        for (const auto& keyframe : seqNode["camera_keyframes"]) {
                            CameraKeyframe frame;
                            frame.target = keyframe["target"].as<std::string>();
                            frame.duration = keyframe["duration"] ? keyframe["duration"].as<float>() : 0.0f;

                            // Fetch position using shared context if target exists
                            if (!frame.target.empty() && sharedContext.HasEntity(frame.target)) {
                                auto targetID = sharedContext.GetEntityID(frame.target);
                                auto* targetObject = m_pGameInstance->GetScene().GetObject(targetID);
                                if (targetObject) {
                                    auto* transform = targetObject->GetComponent<wolf::Transform2D>();
                                    if (transform) {
                                        frame.position = transform->GetGlobalPosition();
                                    }
                                }
                            } else {
                                frame.position = glm::vec2(0.0f, 0.0f); // Default position if target is not found
                            }

                            item.cutscene.push_back(frame);
                        }
                    } 
                    else if (item.type == "combined") {
                        auto& sharedContext = m_pGameInstance->GetSharedContext();

                        // Parse dialogue data
                        if (seqNode["dialogue"]) {
                            DialogueLine line;
                            line.characterName = seqNode["dialogue"]["character"].as<std::string>();
                            line.text = seqNode["dialogue"]["text"].as<std::string>();
                            line.duration = seqNode["dialogue"]["duration"].as<float>();
                            item.dialogue = line;
                        }

                        // Parse cutscene data
                        if (seqNode["cutscene"] && seqNode["cutscene"]["camera_keyframes"]) {
                            for (const auto& keyframe : seqNode["cutscene"]["camera_keyframes"]) {
                                CameraKeyframe frame;
                                frame.target = keyframe["target"].as<std::string>();
                                frame.duration = keyframe["duration"] ? keyframe["duration"].as<float>() : 0.0f;

                                // Fetch position using shared context if target exists
                                if (!frame.target.empty() && sharedContext.HasEntity(frame.target)) {
                                    auto targetID = sharedContext.GetEntityID(frame.target);
                                    auto* targetObject = m_pGameInstance->GetScene().GetObject(targetID);
                                    if (targetObject) {
                                        auto* transform = targetObject->GetComponent<wolf::Transform2D>();
                                        if (transform) {
                                            frame.position = transform->GetGlobalPosition();
                                        }
                                    }
                                } else {
                                    frame.position = glm::vec2(0.0f, 0.0f); // Default position
                                }

                                item.cutscene.push_back(frame);
                            }
                        }
                    }
                    else if (item.type == "fade") {
                        // Parse fade-specific data
                        item.fadeType = seqNode["to"] ? "to" : "from";
                        item.fadeDuration = seqNode["duration"] ? seqNode["duration"].as<float>() : 0.0f;
                    }

                    // Add the item to this sequence's vector
                    sequenceItems.push_back(item);
                }
            }

            // Add the sequence to the map
            m_sequences[sequenceID] = sequenceItems;
        }
    }
}



void DialogueAndCutsceneState::Render(float delta) {

    if (m_currentSequence && m_currentSequenceIndex < m_currentSequence->size()) {
        RenderSequence(delta);
    }
    

    // Render fade overlay if active
    if (m_fadeAlpha > 0.0f) {
        auto* drawList = ImGui::GetBackgroundDrawList();
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        ImColor fadeColor = ImColor(0.0f, 0.0f, 0.0f, m_fadeAlpha); // Black fade
        drawList->AddRectFilled(ImVec2(0, 0), screenSize, fadeColor);
    }
}

    

ImVec2 addImVec2(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}

// Helper to get the current dialogue line
std::string DialogueAndCutsceneState::GetCurrentDialogueLine() const {
    if (m_currentSequence && m_currentSequenceIndex < m_currentSequence->size()) {
        const auto& item = (*m_currentSequence)[m_currentSequenceIndex];
        if (item.type == "dialogue") {
            return item.dialogue.text;
        }
    }
    return "";
}

// Helper to get the current character's name
std::string DialogueAndCutsceneState::GetCurrentCharacterName() const {
    if (m_currentSequence && m_currentSequenceIndex < m_currentSequence->size()) {
        const auto& item = (*m_currentSequence)[m_currentSequenceIndex];
        if (item.type == "dialogue") {
            return item.dialogue.characterName;
        }
    }
    return "";
}

// Handle the continue button press
void DialogueAndCutsceneState::OnContinueButtonPressed() {
    if (!m_showFullText && !m_isLineFinished) {
        m_showFullText = true;
    } else {
        // Advance dialogue and cutscene together if in a "combined" state
        if (m_currentSequence && m_currentSequenceIndex < m_currentSequence->size()) {
            auto& currentItem = (*m_currentSequence)[m_currentSequenceIndex];
            if (currentItem.type == "cutscene" || currentItem.type == "combined") {
                if (m_currentKeyframeIndex < currentItem.cutscene.size()) {
                    // Forcefully complete the current cutscene keyframe
                    auto& targetKeyframe = currentItem.cutscene[m_currentKeyframeIndex];
                    m_cutsceneTimer = targetKeyframe.duration; // Simulate completion
                }
            }
        }

        // Reset progression variables
        m_timeSinceLastKeyframe = 0.0f;
        m_showFullText = false;

        // Advance to the next sequence item
        m_currentSequenceIndex++;
        ResetCutsceneState(); // Ensure state resets for the next item
    }
}


// Handle dialogue ending
void DialogueAndCutsceneState::EndDialogue() {
    m_isDialogueActive = false;
    m_currentSequenceIndex++;
}


void DialogueAndCutsceneState::RenderSequence(float delta) {
    // Ensure we're within valid sequence bounds
    if (!m_currentSequence || m_currentSequenceIndex >= m_currentSequence->size()) {
        return; 
    }

    const auto& currentItem = (*m_currentSequence)[m_currentSequenceIndex];

    // Handle camera rendering (for cutscenes)
    // 
    // NOTE: This caused a bug with light rendering so I axed it (- D'Anyil)
    // Cutscenes still render properly because of the scene's background render
    // 
    // if (currentItem.type == "cutscene" || currentItem.type == "combined") {
    //     auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    //     if (camera) {
    //         // Render the scene with the current camera transformations
    //         m_pGameInstance->GetScene().Render(delta);
    //     }
    // }

    // Handle dialogue rendering
    if (currentItem.type == "dialogue" || currentItem.type == "combined") {
        // Smooth fade-in effect for the dialogue box
        static float fadeOpacity = 0.0f;
        fadeOpacity = std::min(fadeOpacity + delta, 1.0f); // Gradually increase opacity

        // Check if we're on the last line of the dialogue
        bool isLastLine = (m_currentSequence && m_currentSequenceIndex >= m_currentSequence->size() - 1);

        // Screen dimensions
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        float dialogueWidth = screenSize.x * 0.75f; // Set width to 75% of screen
        float baseHeight = 100.0f;                 // Base height for window padding and controls

        // Calculate text height
        const std::string& currentLine = currentItem.dialogue.text;
        float textHeight = ImGui::CalcTextSize(currentLine.c_str(), nullptr, true, dialogueWidth - 100).y;

        // Add extra height if there's a portrait
        bool hasPortrait = false;
        float portraitHeight = 0.0f;
        wolf::Texture* portraitTexture = nullptr;

        const std::string& characterName = currentItem.dialogue.characterName;
        auto it = m_characterPortraits.find(characterName);
        portraitTexture = (it != m_characterPortraits.end()) ? it->second : nullptr;

        if (portraitTexture) {
            hasPortrait = true;
            portraitHeight = 64.0f; // Example portrait height
        }

        // Calculate total window height
        float windowHeight = baseHeight + std::max(textHeight, portraitHeight) + 80.0f; // Add padding for buttons and controls

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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, fadeOpacity));

        ImGui::Begin("EnhancedDialogue", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // Display character name and portrait if available
        if (hasPortrait && portraitTexture) {
            ImGui::Image((void*)(intptr_t)portraitTexture->GetID(), ImVec2(47, 47));
            ImGui::SameLine();
        }

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s:", characterName.c_str());

        // Accent line above text
        ImVec2 start = addImVec2(ImGui::GetCursorScreenPos(), ImVec2(0, -10));
        ImVec2 end = addImVec2(ImGui::GetCursorScreenPos(), ImVec2(windowSize.x * 0.25f, -10));
        drawList->AddLine(start, end, ImColor(0.5f, 0.5f, 0.5f, 0.6f), 2.0f);

        // Display dialogue text
        static float lineFade = 0.0f;
        lineFade = std::min(lineFade + 0.02f, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, lineFade * fadeOpacity);

        if (m_showFullText) {
            ImGui::TextWrapped("%s", currentLine.c_str());
        } else {
            std::string partialText = currentLine.substr(0, static_cast<int>(m_timeSinceLastKeyframe / 0.05f));
            partialText += "|";
            ImGui::TextWrapped("%s", partialText.c_str());
        }
        ImGui::PopStyleVar();

        ImGui::Dummy(ImVec2(0.0f, 20.0f)); // Whitespace below text

        // Centered button layout
        bool showContinueButton = !isLastLine || (isLastLine && !m_isLineFinished);
        float buttonWidth = showContinueButton ? 130.0f : 180.0f;  // Adjust button width
        ImVec2 buttonSize(buttonWidth, 35);

        float totalButtonWidth = showContinueButton ? 420.0f : 2 * buttonWidth;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalButtonWidth) / 2);  // Center buttons
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.75f);

        // Button styling for a polished look
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, fadeOpacity));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, fadeOpacity));

        // Autoplay button
        if (ImGui::Button(m_autoplay ? "Autoplay: ON" : "Autoplay: OFF", buttonSize)) {
            m_autoplay = !m_autoplay;
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);
        ImGui::SameLine();

        // Continue button (if not the last line or line is not finished)
        if (showContinueButton) {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, fadeOpacity));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, fadeOpacity));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, fadeOpacity));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, fadeOpacity));

            if (ImGui::Button("Continue", buttonSize)) {
                m_showFullText = true;
                m_isLineFinished = true;
                // Advance sequence for `combined` only if dialogue is fully displayed
                if (currentItem.type != "combined" || (m_showFullText && m_isLineFinished)) {
                    OnContinueButtonPressed();
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
        }

        // Exit button (always visible)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, fadeOpacity));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, fadeOpacity));

        if (ImGui::Button("Exit", buttonSize)) {
            OnExitButtonPressed();
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);

        ImGui::End();
        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor(3);
    }
}
void DialogueAndCutsceneState::OnExitButtonPressed() {
    // Exit the current sequence by invalidating the index
    if (m_currentSequence) {
        m_currentSequenceIndex = m_currentSequence->size();
    }
}