//-----------------------------------------------------------------------------
// File: HurtboxComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Hurtbox.
//-----------------------------------------------------------------------------

#include "HurtboxComponent.h"

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

int HurtboxComponent::s_iComponentCount = 0;

std::vector<Vertex2D> HurtboxComponent::s_vVerticesVector;

wolf::VertexDeclaration * HurtboxComponent::s_pDecl = nullptr;
wolf::Program *HurtboxComponent::s_pProgram = nullptr;
wolf::VertexBuffer *HurtboxComponent::s_pVB = nullptr;

// Constructor for custom attributes
HurtboxComponent::HurtboxComponent(bool p_type, float p_damage, bool p_doc, bool p_relativity)
{
    this->m_bType = p_type;
    if(p_type == 1)
    {
        this->m_iDamage = p_damage;
    }
    else
    {
        this->m_iDamage = 0;
    }

    this->m_bIsDestroyedOnCollision = p_doc;
    this->m_bIsRelative = p_relativity;

    if (s_pProgram == nullptr)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        s_pProgram = wolf::ProgramManager::CreateProgram("data/shaders/lines.vsh", "data/shaders/lines.fsh");
        s_pVB = wolf::BufferManager::CreateVertexBuffer(vertices.data(), sizeof(Vertex2D) * vertices.size());

        s_pDecl = new wolf::VertexDeclaration();
        s_pDecl->Begin();
        s_pDecl->AppendAttribute(wolf::AT_Position, 2, wolf::CT_Float);
        s_pDecl->SetVertexBuffer(s_pVB);
        s_pDecl->End();
    }
    HurtboxComponent::s_iComponentCount++;
    HurtboxManager::s_iComponentCount++;
}

HurtboxComponent::~HurtboxComponent()
{
    HurtboxComponent::s_iComponentCount--;
    HurtboxManager::s_iComponentCount--;
    if(HurtboxComponent::s_iComponentCount == 0)
    {
        delete HurtboxComponent::s_pDecl;
        HurtboxComponent::s_pDecl = nullptr;
        wolf::ProgramManager::DestroyProgram(HurtboxComponent::s_pProgram);
        HurtboxComponent::s_pProgram = nullptr;
        wolf::BufferManager::DestroyBuffer(HurtboxComponent::s_pVB);
        HurtboxComponent::s_pVB = nullptr;
    }
}

void HurtboxComponent::AddHurtbox(glm::vec2 p_dimensions)
{
    this->m_vHurtboxes.push_back(wolf::Rectangle(glm::vec2(0.0f, 0.0f), p_dimensions));
}

// Get vector of hurtboxes
std::vector<wolf::Rectangle> HurtboxComponent::GetHurtboxes() const
{
    return this->m_vHurtboxes;
}

// Get damage
float HurtboxComponent::GetDamage() const
{
    if(this->m_bType == 1)
    {
        return this->m_iDamage;
    }
    return 0;
}

// Get type
bool HurtboxComponent::GetType() const
{
    return this->m_bType;
}

bool HurtboxComponent::IsDestroyedOnCollision() const
{
    return this->m_bIsDestroyedOnCollision;
}

// Get relativity
bool HurtboxComponent::IsRelative() const
{
    return this->m_bIsRelative;
}

// Add vertices into static vertex array
void HurtboxComponent::FillVertexArray()
{
    glm::vec2 translation = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    for(wolf::Rectangle hurtbox : this->m_vHurtboxes)
    {
        glm::vec2 dimensions = glm::vec2(hurtbox.GetWidth(), hurtbox.GetHeight());
        std::vector<Vertex2D> correctVertices;
        for(Vertex2D vertex : vertices)
        {
            Vertex2D correctVertex;
            correctVertex.x = vertex.x * dimensions.x + translation.x;
            correctVertex.y = vertex.y * dimensions.y + translation.y;
            correctVertices.push_back({correctVertex});
        }
        s_vVerticesVector.insert(s_vVerticesVector.end(), correctVertices.begin(), correctVertices.end());
    }     
}

void HurtboxComponent::DebugDrawAndFlush()
{
    glm::mat4 model = glm::mat4(1.0f);
    s_pProgram->SetUniform("model", model);
    s_pProgram->Bind();
    s_pDecl->Bind();
    
    s_pVB->Bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * s_vVerticesVector.size(), s_vVerticesVector.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINES, 0, s_vVerticesVector.size());

    //std::cout << "Count: " << HitboxComponent::s_iComponentCount << std::endl;
    for(Vertex2D vertex: s_vVerticesVector)
    {
        //std::cout << "X: " << vertex.x << ", Y: " << vertex.y << std::endl;
    }
    s_vVerticesVector.clear();
}

// Return destroy flag
bool HurtboxComponent::IsToBeDestroyed() const
{
    return this->m_bDestroy;
}

// Set destroy flag to true
void HurtboxComponent::RaiseDestroyFlag()
{
    this->m_bDestroy = true;
}

int HurtboxComponent::GetComponentCount()
{
    return HurtboxComponent::s_iComponentCount;
}