//-----------------------------------------------------------------------------
// File: Postprocessor.cpp
// Original Author: Nguyễn Minh Nhật
// Handles postprocessing effects
//-----------------------------------------------------------------------------


#include "Postprocessor.h"
#include "components/LabyrinthManager.h"
#include "TileFireManager.h"
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

const float Postprocessor::SCALED_TILE_SIZE  = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;

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
        s_pPostprocessor->AddShaders("data/shaders/postprocessors/none.vsh","data/shaders/postprocessors/heat_distortion.fsh");
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

// Process the given texture using the effects specified, in order of appearance in the vector
// Will process the same effect twice if so specified 
void Postprocessor::Postprocess(GLuint p_tex, std::vector<Effect> p_effects)
{
    if(p_effects.size() <= 0) return;
    if(m_pSceneCamera == nullptr)
    {
        return;
    }
    
    // Update framebuffers if window size has changed
    glm::vec2 viewSize = m_pSceneCamera->GetViewSize();
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

            case Effect::HEAT_DISTORTION:
            {
                HandleHeatDistortionEffect(currentTex);
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

//-------------------//
//  PRIVATE METHODS  //
//-------------------//

Postprocessor::Postprocessor(wolf::Scene* p_scene)
{
    m_pScene = p_scene;
    m_pSceneCamera = m_pScene->GetActiveCamera();
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

    glGenBuffers(1, &m_uiHeatDistortionSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_uiHeatDistortionSSBO);
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

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glDeleteFramebuffers(1, &m_uiHeatDistortionSSBO);
    m_uiHeatDistortionSSBO = 0;

    m_pScene = nullptr;
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


// Specifically for tile fires
void Postprocessor::HandleHeatDistortionEffect(GLuint p_tex)
{
    // If no fire tiles are active, call HandleNoneEffect() and return
    if(TileFireManager::GetInstance()->GetBurningFireTilesCount() <= 0) 
    {
        HandleNoneEffect(p_tex);
        return;
    }

    std::vector<glm::ivec2> tilePositions = TileFireManager::GetInstance()->GetFireTilePositions(0);

    // If tilePositions is empty, call HandleNoneEffect() and return
    if(tilePositions.size() <= 0) 
    {
        HandleNoneEffect(p_tex);
        return;
    }
        
    // Convert tile positions to world positions
    std::vector<glm::vec2> positions;
    for(int i = 0; i < tilePositions.size(); i++)
    {
        positions.push_back(glm::vec2(tilePositions.at(i).x * SCALED_TILE_SIZE, tilePositions.at(i).y * SCALED_TILE_SIZE));
    }

    // Get relevant camera data
    const float zoom = m_pSceneCamera->GetZoom();
    glm::vec2 viewportSize = m_pSceneCamera->GetViewSize();

    // Bind the framebuffer whose texture will be rendered to
    m_pWriteFBO->Bind();

    // Buffer the SSBO with fire tile data & bind for rendering
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_uiHeatDistortionSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, positions.size() * sizeof(glm::vec2), positions.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_uiHeatDistortionSSBO);

    // Set up the program for the heat distortion effect
    wolf::Program* program = m_vShaderPrograms.at(Effect::HEAT_DISTORTION);

    // Set uniforms    
    program->SetUniform("amplitude", 0.01f * zoom);
    program->SetUniform("frequency", (float)M_PI * 3.0f / zoom);
    program->SetUniform("time", (float)(m_timer.Elapsed()) * 8.0f);
    program->SetUniform("viewport_and_rect", glm::vec4(viewportSize.x, viewportSize.y, SCALED_TILE_SIZE * zoom, SCALED_TILE_SIZE* zoom));
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