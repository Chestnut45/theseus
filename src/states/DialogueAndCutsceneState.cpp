#include "DialogueAndCutsceneState.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "W_TextureManager.h"
#include "W_Input.h"

// Constructor
DialogueAndCutsceneState::DialogueAndCutsceneState(GameStateManager* manager, Theseus* gameInstance, const std::string& yamlFilePath)
    : GameState(manager, gameInstance), m_yamlFilePath(yamlFilePath) {}

// Enter
void DialogueAndCutsceneState::Enter() {
    if (!m_isYAMLLoaded) {
        try {
            LoadFromYAML(m_yamlFilePath);
            m_isYAMLLoaded = true; // Mark as loaded
            std::cout << "DialogueAndCutsceneState: YAML loaded successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error loading YAML: " << e.what() << std::endl;
        }
    } else {
        // std::cout << "DialogueAndCutsceneState: YAML already loaded, skipping reload." << std::endl;
    }
}

// Exit
void DialogueAndCutsceneState::Exit() {
    for (auto& pair : m_characterPortraits) {
        wolf::TextureManager::DestroyTexture(pair.second);
    }
    m_characterPortraits.clear();
    m_dialogueAndCutsceneSequence.clear();
    m_currentSequenceIndex = 0;
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
    if (m_currentSequenceIndex < m_dialogueAndCutsceneSequence.size()) {
        AdvanceSequence(delta);
    } else {
        // Ensure PopState() is called if sequences are complete
        std::cout << "DialogueAndCutsceneState: All sequences complete in Update()." << std::endl;
        m_pStateManager->PopState();
    }
}

void DialogueAndCutsceneState::LoadSequence(const std::string& sequenceID) {
    // Step 1: Enter the state (initialize resources)
    Enter();

    // Step 2: Validate if the YAML loaded correctly
    if (m_dialogueAndCutsceneSequence.empty()) {
        // std::cerr << "DialogueAndCutsceneState: No sequences loaded after YAML parsing." << std::endl;
        return;
    }

    // Step 3: Start the desired sequence
    StartSequence(sequenceID);
}

// StartSequence
void DialogueAndCutsceneState::StartSequence(const std::string& sequenceID) {
    auto it = std::find_if(
        m_dialogueAndCutsceneSequence.begin(),
        m_dialogueAndCutsceneSequence.end(),
        [&sequenceID](const DialogueAndCutsceneItem& item) {
            return item.sequenceID == sequenceID; // Match the sequence ID
        });

    if (it != m_dialogueAndCutsceneSequence.end()) {
        m_currentSequenceIndex = std::distance(m_dialogueAndCutsceneSequence.begin(), it);
        m_timeSinceLastKeyframe = 0.0f;
        m_cutsceneTimer = 0.0f;
        m_currentKeyframeIndex = 0;
    } else {
        // std::cerr << "Sequence ID '" << sequenceID << "' not found!" << std::endl;
    }
}
void DialogueAndCutsceneState::AdvanceSequence(float delta) {
    auto& currentItem = m_dialogueAndCutsceneSequence[m_currentSequenceIndex];

    // Debugging: Print the current sequence type
    std::cout << "Processing sequence at index " << m_currentSequenceIndex 
              << " of type: " << currentItem.type << std::endl;

    // Track the progress of dialogue and cutscene
    bool dialogueFinished = false;
    bool cutsceneFinished = false;

    // Handle dialogue progression
    if (currentItem.type == "dialogue" || currentItem.type == "combined") {
        m_timeSinceLastKeyframe += delta;

        const std::string& currentLine = GetCurrentDialogueLine();
        m_isLineFinished = (m_timeSinceLastKeyframe >= currentLine.length() * 0.05f || m_showFullText);

        // Check user input for skipping
        bool isInputPressed = wolf::Input::IsLMBJustDown() || wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE);
        bool isAnyButtonHovered = ImGui::IsAnyItemHovered();

        if (isInputPressed && !isAnyButtonHovered) {
            if (!m_showFullText && !m_isLineFinished) {
                // Show the full text immediately
                m_showFullText = true;
            } else if (m_isLineFinished) {
                // Mark the dialogue as finished if the line is finished
                dialogueFinished = true;
            }
        }

        // Handle autoplay progression if enabled
        if (m_autoplay && m_isLineFinished && m_timeSinceLastKeyframe > m_autoPlayDelay) {
            dialogueFinished = true;
        }

        // Ensure dialogue sequence is finished only when explicitly marked
        dialogueFinished = dialogueFinished && m_isLineFinished;
    }

    // Handle cutscene progression
    if (currentItem.type == "cutscene" || currentItem.type == "combined") {
        if (m_currentKeyframeIndex >= currentItem.cutscene.size()) {
            // Cutscene finished
            cutsceneFinished = true;
        } else {
            const auto& targetKeyframe = currentItem.cutscene[m_currentKeyframeIndex];
            m_cutsceneTimer += delta;

            // Handle interpolation for camera movement
            float t = glm::clamp(m_cutsceneTimer / targetKeyframe.duration, 0.0f, 1.0f);

            if (m_currentKeyframeIndex > 0) {
                // Interpolate between previous and target keyframe
                const auto& previousKeyframe = currentItem.cutscene[m_currentKeyframeIndex - 1];
                m_currentCameraPosition = glm::mix(previousKeyframe.position, targetKeyframe.position, t);
                m_currentZoomLevel = glm::mix(previousKeyframe.zoom, targetKeyframe.zoom, t);
            } else {
                // Interpolate from the current camera state for the first keyframe
                auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
                if (camera) {
                    glm::vec2 initialPosition = camera->GetPosition();
                    float initialZoom = camera->GetZoom();

                    m_currentCameraPosition = glm::mix(initialPosition, targetKeyframe.position, t);
                    m_currentZoomLevel = glm::mix(initialZoom, targetKeyframe.zoom, t);
                }
            }

            // Apply camera transformations
            auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
            if (camera) {
                camera->SetPosition(m_currentCameraPosition);
                camera->SetZoom(m_currentZoomLevel);
            }

            // Skip to the end of the current keyframe if LMB is pressed
            bool isInputPressed = wolf::Input::IsLMBJustDown() || wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE);
            if (isInputPressed) {
                m_cutsceneTimer = targetKeyframe.duration;
                t = 1.0f; // Force transition to the end of the keyframe
            }

            // Advance to the next keyframe if fully transitioned
            if (t >= 1.0f) {
                m_cutsceneTimer = 0.0f;
                m_currentKeyframeIndex++;
            }

            cutsceneFinished = (m_currentKeyframeIndex >= currentItem.cutscene.size());
        }
    }

    // Combined logic
    if (currentItem.type == "combined") {
        if (dialogueFinished && cutsceneFinished) {
            std::cout << "Combined sequence finished. Advancing to next sequence." << std::endl;

            m_timeSinceLastKeyframe = 0.0f;
            m_showFullText = false;
            m_currentSequenceIndex++;

            // Reset both dialogue and cutscene state for the next item
            ResetCutsceneState();
        }
    } else if ((currentItem.type == "dialogue" && dialogueFinished) ||
               (currentItem.type == "cutscene" && cutsceneFinished)) {
        std::cout << "Sequence finished. Advancing to next sequence." << std::endl;

        m_timeSinceLastKeyframe = 0.0f;
        m_showFullText = false;
        m_currentSequenceIndex++;

        // Reset cutscene state for the next item
        ResetCutsceneState();
    }
}

void DialogueAndCutsceneState::ResetCutsceneState() {
    if (m_currentSequenceIndex < m_dialogueAndCutsceneSequence.size()) {
        auto& nextItem = m_dialogueAndCutsceneSequence[m_currentSequenceIndex];
        std::cout << "Resetting cutscene state for sequence at index " << m_currentSequenceIndex 
                  << " of type: " << nextItem.type << std::endl;

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
            DialogueAndCutsceneItem item;

            // Parse characters
            if (sequenceEntry.second["characters"]) {
                for (const auto& characterNode : sequenceEntry.second["characters"]) {
                    CharacterData character;
                    character.name = characterNode["name"].as<std::string>();
                    character.portraitPath = characterNode["portraitPath"].as<std::string>();
                    // Load portraits into a global map
                    if (m_characterPortraits.find(character.name) == m_characterPortraits.end()) {
                        character.portraitTexture = wolf::TextureManager::CreateTexture(character.portraitPath);
                        m_characterPortraits[character.name] = character.portraitTexture;
                    }
                }
            }

            // Parse the sequence list
            if (sequenceEntry.second["sequence"]) {
                for (const auto& seqNode : sequenceEntry.second["sequence"]) {
                    std::string type = seqNode["type"].as<std::string>();
                    
                    if (type == "dialogue") {
                        DialogueLine line;
                        line.characterName = seqNode["character"].as<std::string>();
                        line.text = seqNode["text"].as<std::string>();
                        line.duration = seqNode["duration"].as<float>();
                        item.type = "dialogue";
                        item.dialogue = line;

                        // Add dialogue to sequence list
                        item.sequenceID = sequenceID;
                        m_dialogueAndCutsceneSequence.push_back(item);
                        item = {}; // Reset the item for subsequent parsing
                    } else if (type == "cutscene") {
                        item.type = "cutscene";
                        auto& sharedContext = m_pGameInstance->GetSharedContext();

                        // Parse camera keyframes
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
                                // Default to (0, 0) if target is not found
                                frame.position = glm::vec2(0.0f, 0.0f);
                            }

                            item.cutscene.push_back(frame);
                        }

                        // Add cutscene to sequence list
                        item.sequenceID = sequenceID;
                        m_dialogueAndCutsceneSequence.push_back(item);
                        item = {}; // Reset the item for subsequent parsing
                    } else if (type == "combined") {
                        item.type = "combined";
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
                                    // Default to (0, 0) if target is not found
                                    frame.position = glm::vec2(0.0f, 0.0f);
                                }

                                item.cutscene.push_back(frame);
                            }
                        }

                        // Add combined item to sequence list
                        item.sequenceID = sequenceID;
                        m_dialogueAndCutsceneSequence.push_back(item);
                        item = {}; // Reset the item for subsequent parsing
                    }
                }
            }
        }
    }
}

void DialogueAndCutsceneState::Render() {
    if (m_currentSequenceIndex < m_dialogueAndCutsceneSequence.size()) {
        RenderSequence();
    }
}

ImVec2 addImVec2(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}

// Helper to get the current dialogue line
std::string DialogueAndCutsceneState::GetCurrentDialogueLine() const {
    if (m_currentSequenceIndex < m_dialogueAndCutsceneSequence.size()) {
        const auto& item = m_dialogueAndCutsceneSequence[m_currentSequenceIndex];
        if (item.type == "dialogue") {
            return item.dialogue.text;
        }
    }
    return "";
}

// Helper to get the current character's name
std::string DialogueAndCutsceneState::GetCurrentCharacterName() const {
    if (m_currentSequenceIndex < m_dialogueAndCutsceneSequence.size()) {
        const auto& item = m_dialogueAndCutsceneSequence[m_currentSequenceIndex];
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
        m_timeSinceLastKeyframe = 0.0f;
        m_showFullText = false;
        m_currentSequenceIndex++;
    }
}

// Handle dialogue ending
void DialogueAndCutsceneState::EndDialogue() {
    m_isDialogueActive = false;
    m_currentSequenceIndex++;
}


void DialogueAndCutsceneState::RenderSequence() {
    // Ensure we're within valid sequence bounds
    if (m_currentSequenceIndex >= m_dialogueAndCutsceneSequence.size()) {
        return;
    }

    const auto& currentItem = m_dialogueAndCutsceneSequence[m_currentSequenceIndex];

    // Handle camera rendering (for cutscenes)
    if (currentItem.type == "cutscene" || currentItem.type == "combined") {
        auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
        if (camera) {
            // Render the scene with the current camera transformations
            m_pGameInstance->GetScene().Render();
        }
    }

    // Handle dialogue rendering
    if (currentItem.type == "dialogue" || currentItem.type == "combined") {
        // Smooth fade-in effect for the dialogue box
        static float fadeOpacity = 0.0f;
        fadeOpacity = std::min(fadeOpacity + 0.05f, 1.0f); // Gradually increase opacity

        // Check if we're on the last line of the dialogue
        bool isLastLine = (m_currentSequenceIndex >= m_dialogueAndCutsceneSequence.size() - 1);

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

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.5f, 0.7f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.95f, fadeOpacity));

        ImGui::Begin("EnhancedDialogue", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // Display character name and portrait if available
        if (hasPortrait && portraitTexture) {
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
        bool showContinueButton = !isLastLine || !m_isLineFinished;
        float buttonWidth = showContinueButton ? 130.0f : 180.0f;  // Adjust button width
        ImVec2 buttonSize(buttonWidth, 35);

        float totalButtonWidth = showContinueButton ? 420.0f : 2 * buttonWidth;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalButtonWidth) / 2);  // Center buttons

        // Button styling for a polished look
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.4f, 0.45f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.3f, 0.35f, fadeOpacity));

        // Autoplay button
        if (ImGui::Button(m_autoplay ? "Autoplay: ON" : "Autoplay: OFF", buttonSize)) {
            m_autoplay = !m_autoplay;
        }

        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        // Continue button (if not the last line or line is not finished)
        if (showContinueButton) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.45f, 0.5f, fadeOpacity));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.55f, 0.6f, fadeOpacity));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));

            if (ImGui::Button("Continue", buttonSize)) {
                m_showFullText = true;
                m_isLineFinished = true;
                // Advance sequence for `combined` only if dialogue is fully displayed
                if (currentItem.type != "combined" || (m_showFullText && m_isLineFinished)) {
                    OnContinueButtonPressed();
                }
            }

            ImGui::PopStyleColor(3);
            ImGui::SameLine();
        }

        // Exit button (always visible)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.4f, 0.45f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.3f, 0.35f, fadeOpacity));

        if (ImGui::Button("Exit", buttonSize)) {
            OnExitButtonPressed();
        }

        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
    }
}
void DialogueAndCutsceneState::OnExitButtonPressed() {
    // Set the sequence index to the end
    m_currentSequenceIndex = m_dialogueAndCutsceneSequence.size();
    std::cout << "DialogueAndCutsceneState: Sequence ended early by user." << std::endl;
}