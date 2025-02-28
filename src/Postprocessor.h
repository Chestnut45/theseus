//-----------------------------------------------------------------------------
// File: Postprocessor.h
// Original Author: Nguyễn Minh Nhật
// Handles postprocessing effects
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>

class Postprocessor
{

public:
    enum Effect
    {
        GRAYSCALE,
        NONE
    };

    static void CreateInstance(wolf::Scene* p_scene);
    static void DestroyInstance();
    static Postprocessor* GetInstance();

    void Postprocess(GLuint p_tex, Effect p_effect);

private:
    Postprocessor(wolf::Scene* p_scene);
    virtual ~Postprocessor();

    void AddShaders(std::string p_vsh, std::string p_fsh);

    static Postprocessor* s_pPostprocessor;

    std::vector<wolf::Program*> m_vShaderPrograms;
    wolf::FrameBuffer * m_pFB = nullptr;

    wolf::VertexDeclaration *m_pVAO = nullptr;
    wolf::VertexBuffer *m_pVBO = nullptr;
    
    wolf::Scene* m_pScene = nullptr;
};