#include "CutSceneState.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

void CutSceneState::Enter() {
    std::cout << "Entering CutSceneState" << std::endl;

    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (!camera) {
        std::cerr << "Error: No active camera found in the scene." << std::endl;
        return;
    }

    // Store initial camera position and zoom
    m_initialPosition = camera->GetPosition();
    m_initialZoom = camera->GetZoom();

    try {
        YAML::Node script = YAML::LoadFile(m_scriptPath);
        YAML::Node cutsceneNode = script["cutscenes"][m_cutsceneID];

        if (!cutsceneNode) {
            std::cerr << "Cutscene ID '" << m_cutsceneID << "' not found in cutscenes.yaml." << std::endl;
            return;
        }

        // Add the initial camera position and zoom as the first keyframe
        m_cameraKeyframes.push_back({m_initialPosition, m_initialZoom, 0.0f});

        // Load additional keyframes from YAML
        for (const auto& frame : cutsceneNode["camera_keyframes"]) {
            glm::vec2 position(frame["position"][0].as<float>(), frame["position"][1].as<float>());
            float zoom = frame["zoom"] ? frame["zoom"].as<float>() : m_initialZoom;  // Use initial zoom if not specified
            float duration = frame["duration"].as<float>();
            m_cameraKeyframes.push_back({position, zoom, duration});
        }

        if (m_cameraKeyframes.empty()) {
            std::cerr << "No keyframes found for cutscene ID '" << m_cutsceneID << "'." << std::endl;
            return;
        }

        m_currentCameraPosition = m_cameraKeyframes[0].position;
        m_currentZoomLevel = m_cameraKeyframes[0].zoom;

    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading cutscene YAML: " << e.what() << std::endl;
    }
}

void CutSceneState::Exit() {
    std::cout << "Exiting CutSceneState" << std::endl;
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

    // Update the active camera's position and zoom level in the scene
    auto* camera = m_pGameInstance->GetScene().GetActiveCamera();
    if (camera) {
        camera->SetPosition(m_currentCameraPosition);
        camera->SetZoom(m_currentZoomLevel);
    }
}
void CutSceneState::Render() {
    // No specific rendering required for camera-only cutscenes
}