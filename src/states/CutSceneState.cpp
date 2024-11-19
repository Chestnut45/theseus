#include "CutSceneState.h"

void CutSceneState::Enter() {
    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (!camera) {
        return;
    }

    // Store initial camera position and zoom
    m_initialPosition = camera->GetPosition();
    m_initialZoom = camera->GetZoom();

    try {
        YAML::Node script = YAML::LoadFile(m_scriptPath);
        YAML::Node cutsceneNode = script["cutscenes"][m_cutsceneID];

        if (!cutsceneNode) {
            return;
        }

        // Add the initial camera position and zoom as the first keyframe
        m_cameraKeyframes.push_back({m_initialPosition, m_initialZoom, 0.0f});

        // Load additional keyframes from YAML
        for (const auto& frame : cutsceneNode["camera_keyframes"]) {
            glm::vec2 position = glm::vec2(0.0f, 0.0f);  // Default position
            std::string target;
            float zoom = m_initialZoom;  // Default zoom value
            float duration = 0.0f;  // Default duration

            // Check if position is defined as an array
            if (frame["position"]) {
                position = glm::vec2(frame["position"][0].as<float>(), frame["position"][1].as<float>());
            }

            // Check for the target value (e.g., Theseus, Gorgon)
            if (frame["target"]) {
                target = frame["target"].as<std::string>();
            }

            // Check if zoom is provided, otherwise, use the default
            if (frame["zoom"]) {
                zoom = frame["zoom"].as<float>();
            }

            // Get the duration of this frame
            if (frame["duration"]) {
                duration = frame["duration"].as<float>();
            }

            // Store the keyframe information
            m_cameraKeyframes.push_back({position, zoom, duration, target});
        }

        if (m_cameraKeyframes.empty()) {
            return;
        }

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
        m_pStateManager->PopState();  // End cutscene when all keyframes are completed
        return;
    }

    // Get the next keyframe target
    CameraKeyframe& targetKeyframe = m_cameraKeyframes[m_currentKeyframe + 1];
    m_timeSinceLastKeyframe += delta;

    // Interpolate position and zoom level
    float t = glm::clamp(m_timeSinceLastKeyframe / targetKeyframe.duration, 0.0f, 1.0f);
    m_currentCameraPosition = glm::mix(m_cameraKeyframes[m_currentKeyframe].position, targetKeyframe.position, t);
    m_currentZoomLevel = glm::mix(m_cameraKeyframes[m_currentKeyframe].zoom, targetKeyframe.zoom, t);

    // Move to the next keyframe if this one is complete
    if (t >= 1.0f) {
        m_timeSinceLastKeyframe = 0.0f;
        m_currentKeyframe++;
        m_currentCameraPosition = targetKeyframe.position;
        m_currentZoomLevel = targetKeyframe.zoom;
    }

    // Check if the target is a GameObject (e.g., Gorgon, Theseus)
    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (camera) {
        if (targetKeyframe.position == glm::vec2(0, 0)) { // Some default value, handle otherwise
            // Handle default case (e.g., specific position)
        } else {
            // If it's a GameObject like Gorgon or Theseus, fetch their position
            auto objectID = m_entityIDs[targetKeyframe.target];
            auto* targetObject = m_pGameInstance->GetScene().GetObject(objectID);
            if (targetObject) {
                auto* transform = targetObject->GetComponent<wolf::Transform2D>();
                if (transform) {
                    m_currentCameraPosition = transform->GetGlobalPosition();
                }
            }
        }

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