#include "W_Input.h"

namespace wolf
{

bool Input::IsKeyDown(int key) { return m_keys[key - GLFW_KEY_SPACE]; }
bool Input::IsKeyJustDown(int key) { return (m_keys[key - GLFW_KEY_SPACE] && !m_prevKeys[key - GLFW_KEY_SPACE]); }
bool Input::IsKeyHeld(int key) { return (m_keys[key - GLFW_KEY_SPACE] && m_prevKeys[key - GLFW_KEY_SPACE]); }
bool Input::IsKeyReleased(int key) { return (!m_keys[key - GLFW_KEY_SPACE] && m_prevKeys[key - GLFW_KEY_SPACE]); }

bool Input::IsLMBDown() { return m_lmbDown; };
bool Input::IsRMBDown() { return m_rmbDown; };
bool Input::IsMMBDown() { return m_mmbDown; };
bool Input::IsLMBJustDown() { return m_lmbDown && !m_prevLmbDown; };
bool Input::IsRMBJustDown() { return m_rmbDown && !m_prevRmbDown; };
bool Input::IsMMBJustDown() { return m_mmbDown && !m_prevMmbDown; };
bool Input::IsLMBHeld() { return m_lmbDown && m_prevLmbDown; };
bool Input::IsRMBHeld() { return m_rmbDown && m_prevRmbDown; };
bool Input::IsMMBHeld() { return m_mmbDown && m_prevMmbDown; };
bool Input::IsLMBReleased() { return !m_lmbDown && m_prevLmbDown; };
bool Input::IsRMBReleased() { return !m_rmbDown && m_prevRmbDown; };
bool Input::IsMMBReleased() { return !m_mmbDown && m_prevMmbDown; };
const glm::vec2& Input::GetMousePos() { return m_mousePos; }
const glm::vec2& Input::GetMouseDelta() { return m_mouseDelta; }
const glm::vec2& Input::GetMouseScroll() { return m_mouseScroll; }

void Input::CaptureMouse()
{
    // Calculate center of screen
    int width, height, x, y;
    glfwGetWindowSize(m_pWindow, &width, &height);
    x = width >> 1;
    y = height >> 1;

    // Set cursor position and update state
    glfwSetCursorPos(m_pWindow, x, y);
    m_mousePos.x = x;
    m_mousePos.y = y;
    m_prevMousePos = m_mousePos;
    m_mouseDelta = glm::vec2(0.0f);

    // Actually capture the mouse
    glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    m_mouseCaptured = true;
}

void Input::ReleaseMouse()
{
    // Release the mouse
    glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    m_mouseCaptured = false;

    // Ensure there are no jumps in delta from releasing the mouse
    double x, y;
    glfwGetCursorPos(m_pWindow, &x, &y);
    m_mousePos.x = x;
    m_mousePos.y = y;
    m_prevMousePos = m_mousePos;
    m_mouseDelta = glm::vec2(0.0f);
}

bool Input::EnableRawMouseMotion()
{
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(m_pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        return true;
    }
    return false;
}

void Input::DisableRawMouseMotion()
{
    glfwSetInputMode(m_pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
}

void Input::_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    m_keys[key - GLFW_KEY_SPACE] = action != GLFW_RELEASE;
}

void Input::_MousePosCallback(GLFWwindow* window, double xpos, double ypos)
{
}

void Input::_MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    m_mouseScroll.x = xoffset;
    m_mouseScroll.y = yoffset;
}

void Input::_Poll()
{
    // Update previous keys
    for (int i = 0; i < NUM_KEYS; i++)
    {
        m_prevKeys[i] = m_keys[i];
        m_keys[i] = glfwGetKey(m_pWindow, i + GLFW_KEY_SPACE) != GLFW_RELEASE;
    }

    // Update mouse buttons
    m_prevLmbDown = m_lmbDown;
    m_prevRmbDown = m_rmbDown;
    m_prevMmbDown = m_mmbDown;
    m_lmbDown = glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    m_rmbDown = glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    m_mmbDown = glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

    // Update mouse positions
    m_prevMousePos = m_mousePos;
    double x, y;
    glfwGetCursorPos(m_pWindow, &x, &y);
    m_mousePos.x = x;
    m_mousePos.y = y;
    m_mouseDelta = m_mousePos - m_prevMousePos;

    // Reset mouse scroll
    m_mouseScroll = glm::vec2(0.0f);
}

void Input::_Setup(GLFWwindow* window)
{
    glfwSetKeyCallback(window, Input::_KeyCallback);
    glfwSetCursorPosCallback(window, Input::_MousePosCallback);
    glfwSetScrollCallback(window, Input::_MouseScrollCallback);
    m_pWindow = window;
}

}