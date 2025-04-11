#pragma once

//-----------------------------------------------------------------------------
// File:			W_App.h
// Original Author:	Gordon Wood
// Modifications:   D'Anyil Landry
//
// Abstract class representing an OpenGL application.
// Modified to handle imgui, audio, and input system setup.
//-----------------------------------------------------------------------------

#include <string>
#include <vector>
#include <W_Types.h>

namespace wolf
{
    class App
    {
    
    // Public API
    public:

        // Construct an app with the given name and initial window dimensions
        App(const std::string& name, int width, int height);
        virtual ~App();

        virtual void Run();
        virtual void Update(float delta) = 0;
        virtual void Render(float delta) = 0;

        // Window functions
        void SetFullscreen(bool p_fullscreen);
        void SetVsync(bool p_vsync);
        
        // Cleanly close the application
        inline void Shutdown() { glfwSetWindowShouldClose(m_pWindow, true); }

    // Visible to derived classes
    protected:

        GLFWwindow *m_pWindow = nullptr;
        std::string m_name;
        int m_width;
        int m_height;
        bool m_windowResized = false;
        bool m_fullscreen = false;
        bool m_vsync = false;

        // Timing
        float m_programLifetime = 0.0f;

        // Displays debug information in an imgui window
        void ShowDebug();
    
    // Implementation details
    private:

        friend void WindowResizeCallback(GLFWwindow* window, int width, int height);

        // Timing
        float m_lastUpdate = 0.0f;
        float m_lastRender = 0.0f;
        float m_lastTime = 0.0f;
        float m_elapsedTime = 0.0f;
        float m_averageFPS = 0.0f;
        float m_sampleAccum = 0.0f;
        uint32_t m_timingFrameCount = 0;
        uint32_t m_totalFrameCount = 0;
        const int m_perfSamplesPerSecond = 60;
        const float m_sampleRate = 1.0f / m_perfSamplesPerSecond;
        const float m_fpsUpdateRate = 1.0f / 2.0f;
        std::vector<float> m_updateSamples;
        std::vector<float> m_renderSamples;
        std::vector<float> m_totalSamples;
    };
}