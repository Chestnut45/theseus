#pragma once

#include <algorithm>
#include <vector>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace wolf
{
    // Input helper functions
    class Input
    {
        // Interface
        public:

            // Constants
            static const int NUM_KEYS = GLFW_KEY_LAST - GLFW_KEY_SPACE;

            // Key inputs
            static bool IsKeyDown(int key);
            static bool IsKeyJustDown(int key);
            static bool IsKeyHeld(int key);
            static bool IsKeyReleased(int key);

            // Mouse inputs
            static bool IsLMBDown();
            static bool IsRMBDown();
            static bool IsMMBDown();
            static bool IsLMBJustDown();
            static bool IsRMBJustDown();
            static bool IsMMBJustDown();
            static bool IsLMBHeld();
            static bool IsRMBHeld();
            static bool IsMMBHeld();
            static bool IsLMBReleased();
            static bool IsRMBReleased();
            static bool IsMMBReleased();
            static bool IsMouseCaptured() { return m_mouseCaptured; };
            static const glm::vec2& GetMousePos();
            static const glm::vec2& GetMouseDelta();
            static const glm::vec2& GetMouseScroll();

            // Mouse config
            static void CaptureMouse();
            static void ReleaseMouse();
            static bool EnableRawMouseMotion();
            static void DisableRawMouseMotion();

        // Data / implementation
        private:

            // Window pointer
            static inline GLFWwindow* m_pWindow = nullptr;

            // Key state
            static inline bool m_keys[NUM_KEYS] = {false};
            static inline bool m_prevKeys[NUM_KEYS] = {false};

            // Mouse state
            static inline bool m_mouseCaptured = false;
            static inline bool m_lmbDown = false;
            static inline bool m_rmbDown = false;
            static inline bool m_mmbDown = false;
            static inline bool m_prevLmbDown = false;
            static inline bool m_prevRmbDown = false;
            static inline bool m_prevMmbDown = false;
            static inline glm::vec2 m_mousePos = glm::vec2(0.0f);
            static inline glm::vec2 m_prevMousePos = glm::vec2(0.0f);
            static inline glm::vec2 m_mouseDelta = glm::vec2(0.0f);
            static inline glm::vec2 m_mouseScroll = glm::vec2(0.0f);

            // GLFW callbacks
            static void _KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
            static void _MousePosCallback(GLFWwindow* window, double xpos, double ypos);
            static void _MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

            // Updates the state not handled by callbacks
            static void _Poll();
            
            // Hook into a GLFW window
            static void _Setup(GLFWwindow* window);

            // Necessary so App can call Poll()
            friend class App;
    };
}