#include "W_App.h"
#include <W_Audio.h>
#include "W_Input.h"
#include "W_Logging.h"

#include <stdlib.h>
#include <stdio.h>

#if _WIN32
#include <windows.h>
#endif

namespace wolf
{

void _ErrorCallback(int error, const char* description)
{
    FatalError(description);
}

void WindowResizeCallback(GLFWwindow* window, int width, int height)
{
    App* pApp = (App*)glfwGetWindowUserPointer(window);
    pApp->m_width = width;
    pApp->m_height = height;
    pApp->m_windowResized = true;
}

App::App(const std::string& name, int width, int height)
  : m_name(name), m_width(width), m_height(height)
{
    if (!glfwInit())
        FatalError("Failed to initialize GLFW");

    Log("GLFW initialized");
    glfwSetErrorCallback(_ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* pMonitorToUse = nullptr;

    m_pWindow = glfwCreateWindow(m_width, m_height, m_name.c_str(), pMonitorToUse, NULL);
    if (!m_pWindow)
        FatalError("GLFW couldn't create window");

    glfwSetWindowUserPointer(m_pWindow, this);
    glfwMakeContextCurrent(m_pWindow);

    // Initialize to default vsync setting
    SetVsync(m_vsync);
    
    // Setup input
    Input::_Setup(m_pWindow);

    // Window resizing callback
    glfwSetWindowSizeCallback(m_pWindow, WindowResizeCallback);

    GLenum err = glewInit();
    if (GLEW_OK != err)
        FatalError((const char*)glewGetErrorString(err));
    else   
        Log("GLEW initialized");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Enable docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup icon font
    io.Fonts->AddFontDefault();
    float baseFontSize = 20.0f;
    float iconFontSize = baseFontSize * 2.0f / 3.0f;
    static const ImWchar iconRange[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.PixelSnapH = true;
    iconConfig.GlyphMinAdvanceX = iconFontSize;
    iconConfig.GlyphOffset.y = 1.5f;
    io.Fonts->AddFontFromFileTTF("thirdparty/" FONT_ICON_FILE_NAME_FAS, iconFontSize, &iconConfig, iconRange);

    // Setup Dear ImGui Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_pWindow, true);
    if (ImGui_ImplOpenGL3_Init())
        Log("ImGui initialized");
    else
        FatalError("ImGui couldn't initialize OpenGL backend");
    
    // Output current OpenGL context version
    Log("OpenGL Context: ", glGetString(GL_VERSION));

    // Initialize audio system
    Audio::_Setup();
}

App::~App()
{
    // Shutdown audio system
    Audio::_Shutdown();
    
    // Shutdown ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    Log("ImGui shutdown");

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
        // Poll for input / events
        Input::_Poll();
        glfwPollEvents();

        // Init ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0U, (const ImGuiViewport*)nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        glfwGetFramebufferSize(m_pWindow, &m_width, &m_height);
        if(m_width != 0 && m_height != 0)
        {
            // Update timing
            double currentTime = glfwGetTime();
            m_elapsedTime = (float)(currentTime - lastTime);
            m_programLifetime += m_elapsedTime;
            lastTime = currentTime;

            // Calculate FPS
            static float timeAccum = 0;
            timeAccum += m_elapsedTime;
            m_timingFrameCount++;
            m_totalFrameCount++;
            while (timeAccum >= m_fpsUpdateRate)
            {
                // 5 updates / second
                m_averageFPS = (float)m_timingFrameCount / timeAccum;
                m_timingFrameCount = 0;
                timeAccum -= m_fpsUpdateRate;
            }
            
            // Update the viewport
            glViewport(0, 0, m_width, m_height);

            // Update
            Update(m_elapsedTime);
            m_lastUpdate = (float)(glfwGetTime() - currentTime);

            // Render
            Render();
            m_lastRender = (float)(glfwGetTime() - m_lastUpdate - currentTime);
        }

        // Render ImGui frame
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update samples
        m_sampleAccum += m_elapsedTime;
        while (m_sampleAccum >= m_sampleRate)
        {
            m_updateSamples.push_back(m_lastUpdate * 1000);
            m_renderSamples.push_back(m_lastRender * 1000);
            m_totalSamples.push_back(m_elapsedTime * 1000);
            if (m_updateSamples.size() > m_perfSamplesPerSecond) m_updateSamples.erase(m_updateSamples.begin());
            if (m_renderSamples.size() > m_perfSamplesPerSecond) m_renderSamples.erase(m_renderSamples.begin());
            if (m_totalSamples.size() > m_perfSamplesPerSecond) m_totalSamples.erase(m_totalSamples.begin());
            m_sampleAccum -= m_sampleRate;
        }

        glfwSwapBuffers(m_pWindow);
    }
}

void App::SetFullscreen(bool p_fullscreen)
{
    if (p_fullscreen)
    {
        // Get primary monitor and enable fullscreen
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_pWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        // Get window monitor and revert to windowed mode
        GLFWmonitor* monitor = glfwGetWindowMonitor(m_pWindow);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_pWindow, NULL, (mode->width - m_width) / 2, (mode->height - m_height) / 2, m_width, m_height, 0);
    }
}

void App::SetVsync(bool p_vsync)
{
    glfwSwapInterval(p_vsync);
}

void App::ShowDebug()
{
    // Default window positioning
    ImGui::SetNextWindowPos(ImVec2((float)(m_width - 256), 0));
    ImGui::SetNextWindowSize(ImVec2(256, 254));
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    
    // Performance monitoring
    ImGui::SeparatorText("Performance:");
    ImGui::Text("Average FPS: %.0f", m_averageFPS);
    ImGui::PlotLines("Update:", m_updateSamples.data(), (int)m_updateSamples.size(), 0, (const char*)nullptr, 0.0f, 16.67f, ImVec2{128.0f, 32.0f});
    ImGui::SameLine();
    ImGui::Text("%.2fms", m_lastUpdate * 1000);
    ImGui::PlotLines("Render:", m_renderSamples.data(), (int)m_renderSamples.size(), 0, (const char*)nullptr, 0.0f, 16.67f, ImVec2{128.0f, 32.0f});
    ImGui::SameLine();
    ImGui::Text("%.2fms", m_lastRender * 1000);
    ImGui::PlotLines("Total:", m_totalSamples.data(), (int)m_totalSamples.size(), 0, (const char*)nullptr, 0.0f, 16.67f, ImVec2{128.0f, 32.0f});
    ImGui::SameLine();
    ImGui::Text("%.2fms", m_elapsedTime * 1000);

    // Window settings
    ImGui::SeparatorText("Window Settings");
    if (ImGui::Checkbox("Fullscreen", &m_fullscreen)) SetFullscreen(m_fullscreen);
    if (ImGui::Checkbox("Vsync", &m_vsync)) SetVsync(m_vsync);

    ImGui::End();
}

}