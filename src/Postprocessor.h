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
        BURNING,
        GRAYSCALE,
        POISONED,
        NONE
    };

    static void CreateInstance(wolf::Scene* p_scene);
    static void DestroyInstance();
    static Postprocessor* GetInstance();

    void Postprocess(GLuint p_tex, std::vector<Effect> p_effects);

private:
    Postprocessor(wolf::Scene* p_scene);
    virtual ~Postprocessor();

    void AddShaders(std::string p_vsh, std::string p_fsh);
    void SwitchFramebuffers();
    void HandleBurningEffect(GLuint p_tex);
    void HandleGrayscaleEffect(GLuint p_tex);
    void HandlePoisonedEffect(GLuint p_tex);
    void HandleNoneEffect(GLuint p_tex);


    static Postprocessor* s_pPostprocessor;

    std::vector<wolf::Program*> m_vShaderPrograms;
    wolf::FrameBuffer * m_pFBO_01 = nullptr;
    wolf::FrameBuffer * m_pFBO_02 = nullptr;
    wolf::FrameBuffer * m_pReadFBO = nullptr;
    wolf::FrameBuffer * m_pWriteFBO = nullptr;

    wolf::VertexDeclaration *m_pVAO = nullptr;
    wolf::VertexBuffer *m_pVBO = nullptr;
    
    wolf::Scene* m_pScene = nullptr;

    wolf::Timer m_timer;
    wolf::RNG m_rng;
};