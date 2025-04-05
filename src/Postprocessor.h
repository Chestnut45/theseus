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

    struct PostprocessData
    {
        friend class Postprocessor;
        
        public:
            std::array<float, Effect::NONE> m_aEffectDurations = {0.0f, 0.0f, 0.0f};
            PostprocessData(std::array<float, Effect::NONE> p_effect_durations)
            {
                m_fActiveEffectsCounter = 0;
                for(int i = 0; i < Effect::NONE; i++)
                {
                    if(p_effect_durations[i] <= 0.0f) continue;
                    
                    m_aEffectDurations[i] = p_effect_durations[i];
                    m_fActiveEffectsCounter++;
                }
            };
            PostprocessData(){};
        
        private:
            float m_fActiveEffectsCounter = 0;
            void Update(float p_dt)
            {
                for(int i = 0; i < Effect::NONE; i++)
                {
                    if(m_aEffectDurations[i] <= 0.0f) continue;
                    
                    m_aEffectDurations[i] -= p_dt;
                    if(m_aEffectDurations[i] <= 0.0f) m_fActiveEffectsCounter--;
                }
            };
    };

    static void CreateInstance(wolf::Scene* p_scene);
    static void DestroyInstance();
    static Postprocessor* GetInstance();

    void Update(float p_dt);

    void Postprocess();
    void Postprocess(GLuint p_tex, std::vector<Effect> p_effects);
    void AddEffect(PostprocessData p_postprocess_data, GLuint p_tex);

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
    std::map<GLuint, PostprocessData> m_mPostprocessData;
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