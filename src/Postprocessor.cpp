

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

        s_pPostprocessor->AddShaders("data/shaders/postprocessors/vertex_shader.vsh","data/shaders/postprocessors/grayscale.fsh");

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

// Process the given texture using the programs specified
void Postprocessor::Postprocess(GLuint p_tex, Effect p_effect)
{
    wolf::Camera2D* camera = m_pScene->GetActiveCamera();
    if(camera == nullptr)
    {
        return;
    }
    
    glm::vec2 viewSize = camera->GetViewSize();
    m_pFB->SetTexSize(viewSize.x, viewSize.y);
    m_pFB->SetWindowSize(viewSize.x, viewSize.y);

    m_pFB->Bind();
    glViewport(0, 0, viewSize.x, viewSize.y);
    wolf::Program* program = m_vShaderPrograms.at(p_effect);
    program->Bind();
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);
    m_pVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
    
    m_pFB->BindDefault();
    m_pFB->Blit();
}

Postprocessor::Postprocessor(wolf::Scene* p_scene)
{

    m_pFB = wolf::BufferManager::CreateFrameBuffer(1920, 1080, 1920, 1080);
    m_pScene = p_scene;

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

    m_pFB = nullptr;

    m_pScene = nullptr;
}

void Postprocessor::AddShaders(std::string p_vsh, std::string p_fsh)
{
    wolf::Program* program = wolf::ProgramManager::CreateProgram(p_vsh, p_fsh);
    m_vShaderPrograms.push_back(program);
}