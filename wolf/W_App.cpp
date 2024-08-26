#include "W_App.h"
#include "W_Input.h"
#include "W_Logging.h"
#include <stdlib.h>
#include <stdio.h>
#if _WIN32
#include <windows.h>
#endif

namespace wolf
{

void _errorCallback(int error, const char* description)
{
    FatalError(description);
}

App::App(const std::string& name, int width, int height)
  : m_name(name), m_width(width), m_height(height)
{
    if (!glfwInit())
        FatalError("Failed to initialize GLFW");

    Log("GLFW initialized");
    glfwSetErrorCallback(_errorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* pMonitorToUse = nullptr;

    m_pWindow = glfwCreateWindow(m_width, m_height, m_name.c_str(), pMonitorToUse, NULL);
    if (!m_pWindow)
        FatalError("Couldn't create window");

    glfwSetWindowUserPointer(m_pWindow, this);
    glfwMakeContextCurrent(m_pWindow);
    glfwSwapInterval(1);
    
    // Setup input
    Input::_Setup(m_pWindow);

    GLenum err = glewInit();
    if (GLEW_OK != err)
        FatalError((const char*)glewGetErrorString(err));
    else   
        Log("GLEW initialized");
    
    // Output current OpenGL context version
    Log("OpenGL Context: ", glGetString(GL_VERSION));
}

App::~App()
{
    // De-init GLFW
    glfwDestroyWindow(m_pWindow);
    glfwTerminate();
    Log("GLFW terminated");
}

void App::Run()
{
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(m_pWindow))
    {
        double currTime = glfwGetTime();
        float elapsedTime = (float)(currTime - lastTime);
        lastTime = currTime;

        // Poll for input / events
        Input::_Poll();
        glfwPollEvents();

        glfwGetFramebufferSize(m_pWindow, &m_width, &m_height);
        if(m_width != 0 && m_height != 0)
        {
            glViewport(0, 0, m_width, m_height);
            Update(elapsedTime);
            Render();
        }

        glfwSwapBuffers(m_pWindow);
    }
}

}