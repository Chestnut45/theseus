#include "W_Sprite2D.h"

#include "W_BufferManager.h"
#include "W_ProgramManager.h"
#include "W_TextureManager.h"

#include "W_Logging.h"

namespace wolf
{

Sprite2D::Sprite2D()
{
    _IncreaseRefCount();
}

Sprite2D::Sprite2D(const std::string& texturePath)
{
    // Load texture
    m_pTexture = wolf::TextureManager::CreateTexture(texturePath);

    // Setup default filter modes
    if (m_pTexture) m_pTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);

    _IncreaseRefCount();
}

Sprite2D::~Sprite2D()
{
    // Update manager's reference count for the loaded texture
    wolf::TextureManager::DestroyTexture(m_pTexture);

    s_refCount--;
    if (s_refCount == 0)
    {
        // Cleanup static shared resources
        wolf::ProgramManager::DestroyProgram(s_pProgram);
        wolf::BufferManager::DestroyBuffer(s_pVertexBuffer);
        wolf::BufferManager::DestroyBuffer(s_pIndexBuffer);
        delete s_pVAO;
    }
}

void Sprite2D::SetTexture(const std::string& texturePath)
{
    // Remove old texture
    if (m_pTexture) wolf::TextureManager::DestroyTexture(m_pTexture);

    // Load new texture
    m_pTexture = wolf::TextureManager::CreateTexture(texturePath);

    // Setup default filter modes
    if (m_pTexture) m_pTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
}

void Sprite2D::SetOrigin(const glm::vec2& origin)
{
    m_origin = origin;
}

void Sprite2D::SetOriginToCenterOfTexture()
{
    if (m_pTexture)
    {
        const glm::vec2 texSize = glm::vec2(m_pTexture->GetWidth(), m_pTexture->GetHeight());
        m_origin.x = texSize.x * 0.5f;
        m_origin.y = texSize.y * 0.5f;
    }
    else
    {
        wolf::Error("Sprite2D texture not loaded, can't center origin");
    }
}

void Sprite2D::Draw(const glm::vec2& position, float rotationRadians, const glm::vec2& scale, const glm::vec3& tint)
{
    // Only render if texture was properly loaded
    if (!m_pTexture || !m_visible) return;

    // Grab the texture size
    const glm::vec2 texSize = glm::vec2(m_pTexture->GetWidth(), m_pTexture->GetHeight());

    // Build model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position - m_origin * scale, 0.0f));
    model = glm::translate(model, glm::vec3(glm::vec2(0.5f) * texSize * scale, 0.0f));
    model = glm::rotate(model, rotationRadians, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(glm::vec2(-0.5f) * texSize * scale, 0.0f));
    model = glm::scale(model, glm::vec3(scale * texSize, 1.0f));

    // Determine tint to use
    const glm::vec3& chosenTint = tint == glm::vec3(-1.0f) ? m_tint : tint;

    // Set uniforms
    s_pProgram->SetUniform("model", model);
    s_pProgram->SetUniform("spriteTint", chosenTint);

    // Bind shader and texture
    s_pProgram->Bind();
    m_pTexture->Bind(0);

    // Issue draw call
    s_pVAO->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void Sprite2D::_IncreaseRefCount()
{
    // Update reference count and initialize static shared resources
    if (s_refCount == 0)
    {
        // Load shader program
        s_pProgram = wolf::ProgramManager::CreateProgram("data/shaders/sprite2d.vs", "data/shaders/sprite2d.fs");

        // Generate vertex buffer data
        float vertexData[16] =
        {
            0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f
        };

        // Create vertex buffer
        s_pVertexBuffer = wolf::BufferManager::CreateVertexBuffer(vertexData, sizeof(vertexData));

        // Generate index buffer data
        unsigned short indexData[6] =
        {
            0, 2, 1, 1, 2, 3
        };

        // Create index buffer
        s_pIndexBuffer = wolf::BufferManager::CreateIndexBuffer(indexData, 6);

        // Create vertex declaration (VAO)
        s_pVAO = new wolf::VertexDeclaration();

        s_pVAO->Begin();
        s_pVAO->SetVertexBuffer(s_pVertexBuffer);
        s_pVAO->SetIndexBuffer(s_pIndexBuffer);
        s_pVAO->AppendAttribute(wolf::Attribute::AT_Position, 2, wolf::ComponentType::CT_Float, 0);
        s_pVAO->AppendAttribute(wolf::Attribute::AT_TexCoord1, 2, wolf::ComponentType::CT_Float, sizeof(float) * 2);
        s_pVAO->End();
    }
    s_refCount++;
}

}