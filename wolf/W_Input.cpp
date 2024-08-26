#include "W_Input.h"

namespace wolf
{

bool Input::IsKeyDown(int key) { return s_keys[key - GLFW_KEY_SPACE]; }
bool Input::IsKeyJustDown(int key) { return (s_keys[key - GLFW_KEY_SPACE] && !s_prevKeys[key - GLFW_KEY_SPACE]); }
bool Input::IsKeyHeld(int key) { return (s_keys[key - GLFW_KEY_SPACE] && s_prevKeys[key - GLFW_KEY_SPACE]); }
bool Input::IsKeyReleased(int key) { return (!s_keys[key - GLFW_KEY_SPACE] && s_prevKeys[key - GLFW_KEY_SPACE]); }

bool Input::IsLMBDown() { return s_lmbDown; };
bool Input::IsRMBDown() { return s_rmbDown; };
bool Input::IsMMBDown() { return s_mmbDown; };
bool Input::IsLMBJustDown() { return s_lmbDown && !s_prevLmbDown; };
bool Input::IsRMBJustDown() { return s_rmbDown && !s_prevRmbDown; };
bool Input::IsMMBJustDown() { return s_mmbDown && !s_prevMmbDown; };
bool Input::IsLMBHeld() { return s_lmbDown && s_prevLmbDown; };
bool Input::IsRMBHeld() { return s_rmbDown && s_prevRmbDown; };
bool Input::IsMMBHeld() { return s_mmbDown && s_prevMmbDown; };
bool Input::IsLMBReleased() { return !s_lmbDown && s_prevLmbDown; };
bool Input::IsRMBReleased() { return !s_rmbDown && s_prevRmbDown; };
bool Input::IsMMBReleased() { return !s_mmbDown && s_prevMmbDown; };
const glm::vec2& Input::GetMousePos() { return s_mousePos; }
const glm::vec2& Input::GetMouseDelta() { return s_mouseDelta; }
const glm::vec2& Input::GetMouseScroll() { return s_mouseScroll; }

void Input::CaptureMouse()
{
    // Calculate center of screen
    int width, height, x, y;
    glfwGetWindowSize(s_pWindow, &width, &height);
    x = width >> 1;
    y = height >> 1;

    // Set cursor position and update state
    glfwSetCursorPos(s_pWindow, x, y);
    s_mousePos.x = x;
    s_mousePos.y = y;
    s_prevMousePos = s_mousePos;
    s_mouseDelta = glm::vec2(0.0f);

    // Actually capture the mouse
    glfwSetInputMode(s_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    s_mouseCaptured = true;
}

void Input::ReleaseMouse()
{
    // Release the mouse
    glfwSetInputMode(s_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    s_mouseCaptured = false;

    // Ensure there are no jumps in delta from releasing the mouse
    double x, y;
    glfwGetCursorPos(s_pWindow, &x, &y);
    s_mousePos.x = x;
    s_mousePos.y = y;
    s_prevMousePos = s_mousePos;
    s_mouseDelta = glm::vec2(0.0f);
}

bool Input::EnableRawMouseMotion()
{
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(s_pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        return true;
    }
    return false;
}

void Input::DisableRawMouseMotion()
{
    glfwSetInputMode(s_pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
}

void Input::_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    s_keys[key - GLFW_KEY_SPACE] = action != GLFW_RELEASE;
}

void Input::_MousePosCallback(GLFWwindow* window, double xpos, double ypos)
{
}

void Input::_MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    s_mouseScroll.x = xoffset;
    s_mouseScroll.y = yoffset;
}

void Input::_Poll()
{
    // Update previous keys
    for (int i = 0; i < NUM_KEYS; i++)
    {
        s_prevKeys[i] = s_keys[i];
        s_keys[i] = glfwGetKey(s_pWindow, i + GLFW_KEY_SPACE) != GLFW_RELEASE;
    }

    // Update mouse buttons
    s_prevLmbDown = s_lmbDown;
    s_prevRmbDown = s_rmbDown;
    s_prevMmbDown = s_mmbDown;
    s_lmbDown = glfwGetMouseButton(s_pWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    s_rmbDown = glfwGetMouseButton(s_pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    s_mmbDown = glfwGetMouseButton(s_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

    // Update mouse positions
    s_prevMousePos = s_mousePos;
    double x, y;
    glfwGetCursorPos(s_pWindow, &x, &y);
    s_mousePos.x = x;
    s_mousePos.y = y;
    s_mouseDelta = s_mousePos - s_prevMousePos;

    // Reset mouse scroll
    s_mouseScroll = glm::vec2(0.0f);
}

void Input::_Setup(GLFWwindow* window)
{
    glfwSetKeyCallback(window, Input::_KeyCallback);
    glfwSetCursorPosCallback(window, Input::_MousePosCallback);
    glfwSetScrollCallback(window, Input::_MouseScrollCallback);
    s_pWindow = window;
}

}