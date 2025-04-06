//-----------------------------------------------------------------------------
// File: Postprocessor.cpp
// Original Author: Nguyễn Minh Nhật
// Handles postprocessing effects
//-----------------------------------------------------------------------------


#include "Postprocessor.h"
#include <VertexDeclarations.h>

const std::vector<TexturedVertex2D> vertices = 
{
    {-1.0f, -1.0f, 0.0f, 0.0f},
    {1.0f, -1.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, 0.0f, 1.0f},
    {-1.0f, -1.0f, 0.0f, 0.0f},
};

Postprocessor* Postprocessor::s_pPostprocessor = nullptr;

//------------------//
//  PUBLIC METHODS  //
//------------------//

void Postprocessor::CreateInstance(wolf::Scene* p_scene)
{
    if(s_pPostprocessor == nullptr)
    {
        s_pPostprocessor = new Postprocessor(p_scene);
        s_pPostprocessor->AddShaders("data/shaders/postprocessors/none.vsh","data/shaders/postprocessors/burning.fsh");
        s_pPostprocessor->AddShaders("data/shaders/postprocessors/none.vsh","data/shaders/postprocessors/grayscale.fsh");
        s_pPostprocessor->AddShaders("data/shaders/postprocessors/none.vsh","data/shaders/postprocessors/poisoned.fsh");
        s_pPostprocessor->AddShaders("data/shaders/postprocessors/none.vsh","data/shaders/postprocessors/none.fsh");

    }
}

void Postprocessor::DestroyInstance()
{
    if(s_pPostprocessor != nullptr)
    {

        delete s_pPostprocessor;
        s_pPostprocessor = nullptr;
    }
}

Postprocessor* Postprocessor::GetInstance()
{
    return s_pPostprocessor;
}


void Postprocessor::Update(float p_dt)
{
    for(auto& [tex, posda] : m_mPostprocessData)
    {
        posda.Update(p_dt);
    }
}

void Postprocessor::Postprocess()
{
    wolf::Camera2D* camera = m_pScene->GetActiveCamera();
    if(camera == nullptr)
    {
        return;
    }

    // Update framebuffers if window size has changed
    glm::vec2 viewSize = camera->GetViewSize();
    m_pFBO_01->SetTexSize(viewSize.x, viewSize.y);
    m_pFBO_01->SetWindowSize(viewSize.x, viewSize.y);
    m_pFBO_02->SetTexSize(viewSize.x, viewSize.y);
    m_pFBO_02->SetWindowSize(viewSize.x, viewSize.y);

    for(auto const& [tex, posda] : m_mPostprocessData)
    {
        GLuint currentTex = tex;
        
        if(posda.m_fActiveEffectsCounter <= 0)
        {
            HandleNoneEffect(currentTex);
            SwitchFramebuffers();
        }
        
        else
        {
            for(int i = 0; i < Effect::NONE; i++)
            {
                float duration = posda.m_aEffectDurations[i];

                // Skip if duration expired
                if(duration <= 0.0f) continue;

                // Apply effects
                Effect effect = (Effect)i;

                switch (effect)
                {
                    case Effect::BURNING:
                    {
                        HandleBurningEffect(currentTex);
                        break;
                    }

                    case Effect::GRAYSCALE:
                    {
                        
                        HandleGrayscaleEffect(currentTex);
                        break;
                    }

                    case Effect::POISONED:
                    {
                        
                        HandlePoisonedEffect(currentTex);
                        break;
                    }
                    
                    default:
                    {
                        HandleNoneEffect(currentTex);   // Only implemented to ensure SwitchFramebuffers() does not break when defaulted - Should NEVER be called
                        break;       
                    }
                }                
                // Switch framebuffers after every effect
                SwitchFramebuffers();
                // Set current texture to be the texture that was just rendered to
                currentTex = m_pReadFBO->GetTextureID();
            }
        }

        // Render to screen
        m_pReadFBO->Blit();
    }
}

void Postprocessor::AddEffect(PostprocessData p_postprocess_data, GLuint p_tex)
{
    if(p_tex <= 0) return;

    // Check if texture is registered
    auto itr = m_mPostprocessData.find(p_tex);

    // If texture is not registered
    if(itr == m_mPostprocessData.end())
    {
        // Create new map object
        m_mPostprocessData.insert({p_tex, p_postprocess_data});
        return;
    }

    // If texture is already registered
    else
    {
        PostprocessData posda = p_postprocess_data;
        for(int i = 0; i < Effect::NONE; i++)
        {
            // If input duration is less than 0, keep the old duration
            if(p_postprocess_data.m_aEffectDurations[i] < 0.0f)
            {
                posda.m_aEffectDurations[i] = m_mPostprocessData[p_tex].m_aEffectDurations[i];
            }
        }
        m_mPostprocessData[p_tex] = posda;
    }

}

//-------------------//
//  PRIVATE METHODS  //
//-------------------//

Postprocessor::Postprocessor(wolf::Scene* p_scene)
{
    m_pScene = p_scene;
    m_timer.Start();
    m_rng.NextInt(0, 1);


    // Create framebuffers
    m_pFBO_01 = wolf::BufferManager::CreateFrameBuffer(1920, 1080, 1920, 1080);
    m_pFBO_02 = wolf::BufferManager::CreateFrameBuffer(1920, 1080, 1920, 1080);
    m_pReadFBO = m_pFBO_01;
    m_pWriteFBO = m_pFBO_02;

    // Create VBO
    m_pVBO = wolf::BufferManager::CreateVertexBuffer(vertices.data(), sizeof(TexturedVertex2D) * vertices.size());

    // Create VAO
    m_pVAO = new wolf::VertexDeclaration();
    m_pVAO->Begin();
    m_pVAO->SetVertexBuffer(m_pVBO);
    m_pVAO->AppendAttribute(wolf::Attribute::AT_Position, 2, wolf::ComponentType::CT_Float, 0);
    m_pVAO->AppendAttribute(wolf::Attribute::AT_TexCoord1, 2, wolf::ComponentType::CT_Float, sizeof(float) * 2);
    m_pVAO->End();
}

Postprocessor::~Postprocessor()
{
    for(auto program : m_vShaderPrograms)
    {
        wolf::ProgramManager::DestroyProgram(program);
        program = nullptr;
    }

    m_vShaderPrograms.clear();

    wolf::BufferManager::DestroyBuffer(m_pFBO_01);
    m_pFBO_01 = nullptr;
    wolf::BufferManager::DestroyBuffer(m_pFBO_02);
    m_pFBO_02 = nullptr;
    
    m_pReadFBO = nullptr;
    m_pWriteFBO = nullptr;

    m_pScene = nullptr;
}

void Postprocessor::Postprocess(GLuint p_tex, std::vector<Effect> p_effects)
{
    if(p_effects.size() <= 0) return;
    wolf::Camera2D* camera = m_pScene->GetActiveCamera();
    if(camera == nullptr)
    {
        return;
    }
    
    // Update framebuffers if window size has changed
    glm::vec2 viewSize = camera->GetViewSize();
    m_pFBO_01->SetTexSize(viewSize.x, viewSize.y);
    m_pFBO_01->SetWindowSize(viewSize.x, viewSize.y);
    m_pFBO_02->SetTexSize(viewSize.x, viewSize.y);
    m_pFBO_02->SetWindowSize(viewSize.x, viewSize.y);

    // Set initual value of current texture
    GLuint currentTex = p_tex;

    // Iterate & apply effects
    for(auto effect: p_effects)
    {
        switch (effect)
        {
            case Effect::BURNING:
            {
                HandleBurningEffect(currentTex);
                break;
            }

            case Effect::GRAYSCALE:
            {
                HandleGrayscaleEffect(currentTex);
                break;
            }

            case Effect::POISONED:
            {
                HandlePoisonedEffect(currentTex);
                break;
            }
            
            default:
                HandleNoneEffect(currentTex);   // Only implemented to ensure SwitchFramebuffers() does not break when defaulted - Should NEVER be called
                break;
        }

        // Switch framebuffers after every effect
        SwitchFramebuffers();

        // Set current texture to be the texture that was just rendered to
        currentTex = m_pReadFBO->GetTextureID();
    }

    // Render to screen
    m_pReadFBO->Blit();
}


void Postprocessor::AddShaders(std::string p_vsh, std::string p_fsh)
{
    wolf::Program* program = wolf::ProgramManager::CreateProgram(p_vsh, p_fsh);
    m_vShaderPrograms.push_back(program);
}

void Postprocessor::SwitchFramebuffers()
{
    wolf::FrameBuffer* temp = m_pReadFBO;
    m_pReadFBO = m_pWriteFBO;
    m_pWriteFBO = temp;
}

void Postprocessor::HandleBurningEffect(GLuint p_tex)
{
    // Bind the framebuffer whose texture will be rendered to
    m_pWriteFBO->Bind();

    // Set up the program for the burning effect & specify uniforms
    wolf::Program* program = m_vShaderPrograms.at(Effect::BURNING);
    program->Bind();
    program->SetUniform("fireGradientRate", 16.0f);
    program->SetUniform("fireSineAmplitude", 0.016f);
    program->SetUniform("fireSineFrequency", 32.0f);
    program->SetUniform("fireSineMidline", 0.32f);
    program->SetUniform("time", (float)m_timer.Elapsed() * 4.0f);
    program->SetUniform("burnRGB", glm::vec3(1.2f, 0.4f, 0.04f));   // Red tint of screen

    // Bind the texture to apply postprocessing effects to
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);

    // Bind the VAO
    m_pVAO->Bind();

    // Render
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    
    // Unbind the VAO
    glBindVertexArray(0);

    // Bind default framebuffer (screen)
    wolf::FrameBuffer::BindDefault();
}

void Postprocessor::HandleGrayscaleEffect(GLuint p_tex)
{
    // Bind the framebuffer whose texture will be rendered to
    m_pWriteFBO->Bind();
    
    // Set up the program for the grayscale effect
    wolf::Program* program = m_vShaderPrograms.at(Effect::GRAYSCALE);
    program->Bind();

    // Bind the texture to apply postprocessing effects to
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);

    // Bind the VAO
    m_pVAO->Bind();

    // Render
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    // Unbind the VAO
    glBindVertexArray(0);

    // Bind default framebuffer (screen)
    wolf::FrameBuffer::BindDefault();
}

void Postprocessor::HandlePoisonedEffect(GLuint p_tex)
{
    // Bind the framebuffer whose texture will be rendered to
    m_pWriteFBO->Bind();

    // Set up the program for the poisoned effect & specify uniforms
    wolf::Program* program = m_vShaderPrograms.at(Effect::POISONED);
    program->Bind();
    program->SetUniform("time", (float)(m_timer.Elapsed()) * 6.0f);
    program->SetUniform("amplitude", 0.01f);
    program->SetUniform("frequency", (float)M_PI * 3.0f);

    // Bind the texture to apply postprocessing effects to
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);

    // Bind the VAO
    m_pVAO->Bind();

    // Render
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    // Unbind the VAO
    glBindVertexArray(0);

    // Bind default framebuffer (screen)
    wolf::FrameBuffer::BindDefault();
}

void Postprocessor::HandleNoneEffect(GLuint p_tex)
{
    // Bind the framebuffer whose texture will be rendered to
    m_pWriteFBO->Bind();
    // Set up the program
    wolf::Program* program = m_vShaderPrograms.at(Effect::NONE);
    program->Bind();

    // Bind the texture to apply postprocessing effects to
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);

    // Bind the VAO
    m_pVAO->Bind();

    // Render
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    // Unbind the VAO
    glBindVertexArray(0);

    // Bind default framebuffer (screen)
    wolf::FrameBuffer::BindDefault();
}