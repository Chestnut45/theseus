#include "CutSceneState.h"

void CutSceneState::Enter() {
    auto& sharedContext = m_pGameInstance->GetSharedContext();  // Access shared context from Theseus

    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (!camera) {
        std::cerr << "Error: No active camera found!" << std::endl;
        return;
    }

    // Store initial camera position and zoom
    m_initialPosition = camera->GetPosition();
    m_initialZoom = camera->GetZoom();

    try {
        // Load cutscene data from YAML file
        YAML::Node script = YAML::LoadFile(m_scriptPath);
        YAML::Node cutsceneNode = script["cutscenes"][m_cutsceneID];

        if (!cutsceneNode) {
            std::cerr << "Error: Cutscene with ID '" << m_cutsceneID << "' not found in the script!" << std::endl;
            return;
        }

        // Add the initial camera position and zoom as the first keyframe
        m_cameraKeyframes.push_back({m_initialPosition, m_initialZoom, 0.0f, ""});

        // Process camera keyframes from YAML
        for (const auto& frame : cutsceneNode["camera_keyframes"]) {
            glm::vec2 position = glm::vec2(0.0f, 0.0f);  // Default position
            std::string target;
            float zoom = m_initialZoom;  // Default zoom
            float duration = 0.0f;       // Default duration

            // Check for a target entity (e.g., "Theseus", "Gorgon")
            if (frame["target"]) {
                target = frame["target"].as<std::string>();
                if (sharedContext.HasEntity(target)) {
                    auto targetID = sharedContext.GetEntityID(target);
                    auto* targetObject = m_pGameInstance->GetScene().GetObject(targetID);
                    if (targetObject) {
                        auto* transform = targetObject->GetComponent<wolf::Transform2D>();
                        if (transform) {
                            position = transform->GetGlobalPosition();  // Use entity's global position
                        }
                    }
                } else {
                    std::cerr << "Warning: Entity '" << target << "' not found in shared context!" << std::endl;
                }
            }

            // Load zoom and duration if provided
            if (frame["zoom"]) {
                zoom = frame["zoom"].as<float>();
            }
            if (frame["duration"]) {
                duration = frame["duration"].as<float>();
            }

            // Store the keyframe
            m_cameraKeyframes.push_back({position, zoom, duration, target});
        }

        if (m_cameraKeyframes.empty()) {
            std::cerr << "Warning: No valid keyframes found for cutscene '" << m_cutsceneID << "'!" << std::endl;
            return;
        }

        // Initialize camera position and zoom to the first keyframe
        m_currentCameraPosition = m_cameraKeyframes[0].position;
        m_currentZoomLevel = m_cameraKeyframes[0].zoom;

    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading cutscene YAML: " << e.what() << std::endl;
    }
}

void CutSceneState::Exit() {
    m_cameraKeyframes.clear();

    // Re-enable camera follow and restore the initial zoom level
    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (camera) {
        camera->SetZoom(m_initialZoom);  // Restore initial zoom
        camera->SetFollowSpeed(2.0f);    // Restore follow speed if needed
    }
}
void CutSceneState::Update(float delta) {
    if (m_cameraKeyframes.empty() || m_currentKeyframe >= m_cameraKeyframes.size() - 1) {
        // End cutscene and notify DialogueState
        wolf::EventManager::TriggerEvent(DialogueResumeEvent(m_cutsceneID));
        m_pStateManager->PopState();  // Return to DialogueState
        return;
    }

    // Handle camera interpolation logic
    CameraKeyframe& targetKeyframe = m_cameraKeyframes[m_currentKeyframe + 1];
    m_timeSinceLastKeyframe += delta;

    // Interpolate position and zoom level
    float t = glm::clamp(m_timeSinceLastKeyframe / targetKeyframe.duration, 0.0f, 1.0f);
    m_currentCameraPosition = glm::mix(m_cameraKeyframes[m_currentKeyframe].position, targetKeyframe.position, t);
    m_currentZoomLevel = glm::mix(m_cameraKeyframes[m_currentKeyframe].zoom, targetKeyframe.zoom, t);

    // Advance to the next keyframe
    if (t >= 1.0f) {
        m_timeSinceLastKeyframe = 0.0f;
        m_currentKeyframe++;
    }

    // Apply camera transformations
    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (camera) {
        camera->SetPosition(m_currentCameraPosition);
        camera->SetZoom(m_currentZoomLevel);
    }
}
void CutSceneState::Render() {
    // No specific rendering required for camera-only cutscenes
}

void CutSceneState::BackgroundRender(){
    // Render the cutscene (e.g., camera effects) in the background
    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (camera) {
        m_pGameInstance->GetScene().Render();  // Ensure the scene reflects current camera state
    }
}
void CutSceneState::BackgroundUpdate(float delta){
    // Continue camera keyframe updates
    Update(delta);
}