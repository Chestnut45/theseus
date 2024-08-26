#pragma once
#define GLEW_NO_GLU
#include "W_Types.h"
#include <string>

namespace wolf
{
    class App
    {
    public:
        App(const std::string& name, int width, int height);
        virtual ~App();

        virtual void Run();
        virtual void Update(float delta) = 0;
        virtual void Render() = 0;
        
        inline void Shutdown() { glfwSetWindowShouldClose(m_pWindow, true); }

    protected:
        GLFWwindow *m_pWindow = nullptr;
        std::string m_name;
        int m_width;
        int m_height;
    };
}