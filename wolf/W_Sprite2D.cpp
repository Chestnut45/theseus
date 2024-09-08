#include "W_Sprite2D.h"

#include "W_BufferManager.h"
#include "W_ProgramManager.h"
#include "W_TextureManager.h"

namespace wolf
{

Sprite2D::Sprite2D(const std::string& texturePath)
{
    // Load texture
    m_pTexture = wolf::TextureManager::CreateTexture(texturePath);

    // Setup default filter and wrap modes
    m_pTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);

    // Set size
    m_size = glm::vec2(m_pTexture->GetWidth(), m_pTexture->GetHeight());

    // Update reference count and initialize static shared resources
    if (refCount == 0)
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
    refCount++;
}

Sprite2D::~Sprite2D()
{
    // Update manager's reference count for the loaded texture
    wolf::TextureManager::DestroyTexture(m_pTexture);

    refCount--;
    if (refCount == 0)
    {
        // Cleanup static shared resources
        wolf::ProgramManager::DestroyProgram(s_pProgram);
        wolf::BufferManager::DestroyBuffer(s_pVertexBuffer);
        wolf::BufferManager::DestroyBuffer(s_pIndexBuffer);
        delete s_pVAO;
    }
}

void Sprite2D::Draw(const glm::vec2& worldPosition, float rotationDegrees, const glm::vec2& scale, const glm::vec3& color)
{
    // Bind sprite shader
    s_pProgram->Bind();

    // Bind sprite texture to texture unit 0
    m_pTexture->Bind(0);

    // Initialize with identity matrix
    glm::mat4 model = glm::mat4(1.0f);

    // Build model matrix
    model = glm::translate(model, glm::vec3(worldPosition, 0.0f));
    model = glm::translate(model, glm::vec3(glm::vec2(0.5f) * m_size * scale, 0.0f));
    model = glm::rotate(model, glm::radians(rotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(glm::vec2(-0.5f) * m_size * scale, 0.0f));
    model = glm::scale(model, glm::vec3(scale * m_size, 1.0f));

    // Set uniforms
    s_pProgram->SetUniform("model", model);
    s_pProgram->SetUniform("spriteTint", color);

    // Issue draw call
    s_pVAO->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

}