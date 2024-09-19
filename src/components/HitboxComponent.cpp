//-----------------------------------------------------------------------------
// File: HitboxComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Hitbox.
//-----------------------------------------------------------------------------

#include "HitboxComponent.h"

const std::vector<Vertex2D> vertices = 
{
    {0.0f, 0.0f},
    {0.0f, 1.0f},

    {0.0f, 1.0f},
    {1.0f, 1.0f},

    {1.0f, 1.0f},
    {1.0f, 0.0f},

    {1.0f, 0.0f},
    {0.0f, 0.0f}
};

int HitboxComponent::s_iCounter = 0;

std::vector<Vertex2D> HitboxComponent::s_vVerticesVector;

wolf::VertexDeclaration * HitboxComponent::s_pDecl = nullptr;
wolf::Program *HitboxComponent::s_pProgram = nullptr;
wolf::VertexBuffer *HitboxComponent::s_pVB = nullptr;

HitboxComponent::HitboxComponent()
{
    this->m_pHitbox = new wolf::Rectangle(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f));

    s_iCounter++;
    if (s_pProgram == nullptr)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        s_pProgram = wolf::ProgramManager::CreateProgram("data/shaders/lines.vsh", "data/shaders/lines.fsh");
        s_pVB = wolf::BufferManager::CreateVertexBuffer(vertices.data(), sizeof(Vertex2D) * 8);

        s_pDecl = new wolf::VertexDeclaration();
        s_pDecl->Begin();
        s_pDecl->AppendAttribute(wolf::AT_Position, 2, wolf::CT_Float);
        s_pDecl->SetVertexBuffer(s_pVB);
        s_pDecl->End();
    }
}

// Constructor for custom attributes
HitboxComponent::HitboxComponent(glm::vec2 p_dimensions,bool p_doc, bool p_relativity)
{
    this->m_pHitbox = new wolf::Rectangle(glm::vec2(0.0f, 0.0f), p_dimensions);
    this->m_bIsDestroyedOnCollision = p_doc;
    this->m_bIsRelative = p_relativity;
    
    s_iCounter++;
    if (s_pProgram == nullptr)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        s_pProgram = wolf::ProgramManager::CreateProgram("data/shaders/lines.vsh", "data/shaders/lines.fsh");
        s_pVB = wolf::BufferManager::CreateVertexBuffer(vertices.data(), sizeof(Vertex2D) * 8);

        s_pDecl = new wolf::VertexDeclaration();
        s_pDecl->Begin();
        s_pDecl->AppendAttribute(wolf::AT_Position, 2, wolf::CT_Float);
        s_pDecl->SetVertexBuffer(s_pVB);
        s_pDecl->End();
    }
}

// Destructor
HitboxComponent::~HitboxComponent()
{
    this->m_pHitbox = nullptr;

    s_iCounter--;
    // delete this->m_pDecl;
    // this->m_pDecl = nullptr;
    // wolf::ProgramManager::DestroyProgram(this->m_pProgram);
    // this->m_pProgram = nullptr;
    // wolf::BufferManager::DestroyBuffer(this->m_pVB);
    // this->m_pVB = nullptr;
}

// Get wolf::Rectangle hitbox

wolf::Rectangle* HitboxComponent::GetHitbox()
{
    return this->m_pHitbox;
}

// Get dimensions
glm::vec2 HitboxComponent::GetDimensions() const
{
    glm::vec2 dimensions =glm::vec2(this->m_pHitbox->GetWidth(), this->m_pHitbox->GetHeight());
    if(this->m_bIsRelative)
    {
        glm::vec2 scale = GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
        dimensions *= scale;
    }
    return dimensions;
}


bool HitboxComponent::IsDestroyedOnCollision() const
{
    return this->m_bIsDestroyedOnCollision;
}

// Get relativity
bool HitboxComponent::IsRelative() const
{
    return this->m_bIsRelative;
}

// Get destroy flag
 bool HitboxComponent::IsToBeDestroyed() const
 {
    return this->m_bDestroy;
 }

// Set destroy flag to true
void HitboxComponent::RaiseDestroyFlag()
{
    this->m_bDestroy = true;
}

// Render
void HitboxComponent::FillVertexArray()
{
    s_vVerticesVector.insert(s_vVerticesVector.end(), vertices.begin(), vertices.end());
}

void HitboxComponent::DebugDrawAndFlush()
{
    glm::mat4 model = glm::mat4(1.0f);
    s_pProgram->SetUniform("model", model);
    s_pProgram->Bind();
    s_pDecl->Bind();
    glDrawArrays(GL_LINES, 0, 8);

    s_vVerticesVector.clear();
}