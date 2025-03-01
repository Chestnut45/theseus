

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

void Postprocessor::CreateInstance(wolf::Scene* p_scene)
{
    if(s_pPostprocessor == nullptr)
    {
        s_pPostprocessor = new Postprocessor(p_scene);
        s_pPostprocessor->AddShaders("data/shaders/postprocessors/none.vsh","data/shaders/postprocessors/burning.fsh");
        s_pPostprocessor->AddShaders("data/shaders/postprocessors/none.vsh","data/shaders/postprocessors/grayscale.fsh");
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

// Process the given texture using the effects specified
void Postprocessor::Postprocess(GLuint p_tex, std::vector<Effect> p_effects)
{
    if(p_effects.size() <= 0) return;
    wolf::Camera2D* camera = m_pScene->GetActiveCamera();
    if(camera == nullptr)
    {
        return;
    }
    
    glm::vec2 viewSize = camera->GetViewSize();
    m_pFBO_01->SetTexSize(viewSize.x, viewSize.y);
    m_pFBO_01->SetWindowSize(viewSize.x, viewSize.y);
    m_pFBO_02->SetTexSize(viewSize.x, viewSize.y);
    m_pFBO_02->SetWindowSize(viewSize.x, viewSize.y);

    GLuint currentTex = p_tex;
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
            
            default:
                HandleNoneEffect(currentTex);   // Only implemented to ensure SwitchFramebuffers() does not break when defaulted - Should NEVER be called
                break;
        }

        SwitchFramebuffers();
        currentTex = m_pReadFBO->GetTextureID();
    }
    m_pReadFBO->Blit();
}

Postprocessor::Postprocessor(wolf::Scene* p_scene)
{
    m_pScene = p_scene;

    m_pFBO_01 = wolf::BufferManager::CreateFrameBuffer(1920, 1080, 1920, 1080);
    m_pFBO_02 = wolf::BufferManager::CreateFrameBuffer(1920, 1080, 1920, 1080);
    m_pReadFBO = m_pFBO_01;
    m_pWriteFBO = m_pFBO_02;

    // Create vertex buffer
    m_pVBO = wolf::BufferManager::CreateVertexBuffer(vertices.data(), sizeof(TexturedVertex2D) * vertices.size());

    // Create vertex declaration (VAO)
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
    m_pWriteFBO->Bind();
    wolf::Program* program = m_vShaderPrograms.at(Effect::BURNING);
    program->Bind();
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);
    m_pVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
    m_pWriteFBO->BindDefault();
}

void Postprocessor::HandleGrayscaleEffect(GLuint p_tex)
{
    m_pWriteFBO->Bind();
    wolf::Program* program = m_vShaderPrograms.at(Effect::GRAYSCALE);
    program->Bind();
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);
    m_pVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
    m_pWriteFBO->BindDefault();
}

void Postprocessor::HandleNoneEffect(GLuint p_tex)
{
    m_pWriteFBO->Bind();
    wolf::Program* program = m_vShaderPrograms.at(Effect::NONE);
    program->Bind();
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);
    m_pVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
    m_pWriteFBO->BindDefault();
}