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
HurtboxComponent::HurtboxComponent(ColliderType p_collider_type, bool p_doc, bool p_relativity)
{
    this->m_ColliderType = p_collider_type;
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
}

HurtboxComponent::~HurtboxComponent()
{
    HurtboxComponent::s_iComponentCount--;
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

void HurtboxComponent::AddHurtbox(glm::vec2 p_dimensions, glm::vec2 p_offset)
{
    this->m_vHurtboxes.push_back(wolf::Rectangle(p_offset, p_dimensions));
}

// Get vector of hurtboxes
std::vector<wolf::Rectangle> HurtboxComponent::GetHurtboxes() const
{
    return this->m_vHurtboxes;
}

bool HurtboxComponent::IsHitbox() const
{
    if(this->m_ColliderType == ColliderType::HITBOX || this->m_ColliderType == ColliderType::HITHURTDD, this->m_ColliderType == ColliderType::HITHURTDR)
    {
        return true;
    }
    return false;
}

bool HurtboxComponent::IsHurtbox() const
{
    if(this->m_ColliderType == ColliderType::HURTBOXDD || this->m_ColliderType == ColliderType::HURTBOXDR || this->m_ColliderType == ColliderType::HITHURTDD, this->m_ColliderType == ColliderType::HITHURTDR)
    {
        return true;
    }
    return false;
}

bool HurtboxComponent::IsHurtboxDamageDealer() const
{
    if(this->m_ColliderType == ColliderType::HURTBOXDD || this->m_ColliderType == ColliderType::HITHURTDD)
    {
        return true;
    }
    return false;
}

bool HurtboxComponent::IsHurtboxDamageReceiver() const
{
    if(this->m_ColliderType == ColliderType::HURTBOXDR || this->m_ColliderType == ColliderType::HITHURTDR)
    {
        return true;
    }
    return false;
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

// get damage
float HurtboxComponent::GetDamage() const
{
    if(this->IsHurtboxDamageDealer())
    {
        return this->m_fDamage;
    }
    return 0.0f;
}

// Get collider type
HurtboxComponent::ColliderType HurtboxComponent::GetColliderType() const
{
    return this->m_ColliderType;
}

// Fill vertex array with vertices of instance
void HurtboxComponent::FillVertexArray()
{
    glm::vec2 translation = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    for(wolf::Rectangle hurtbox : this->m_vHurtboxes)
    {
        glm::vec2 dimensions = glm::vec2(hurtbox.GetWidth(), hurtbox.GetHeight());
        glm::vec2 offset = hurtbox.GetPosition();

        std::vector<Vertex2D> correctVertices;
        for(Vertex2D vertex : vertices)
        {
            Vertex2D correctVertex;
            if(this->m_bIsRelative)
            {
                glm::vec2 scale = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                correctVertex.x = vertex.x * dimensions.x * scale.x + translation.x + offset.x * scale.x;
                correctVertex.y = vertex.y * dimensions.y * scale.y + translation.y + offset.y * scale.y;
            }
            else{
                correctVertex.x = vertex.x * dimensions.x + translation.x + offset.x;
                correctVertex.y = vertex.y * dimensions.y + translation.y + offset.y;
            }  
            
            correctVertices.push_back({correctVertex});
        }
        s_vVerticesVector.insert(s_vVerticesVector.end(), correctVertices.begin(), correctVertices.end());
    }     
}

// Draw boundaries & flush vertex vector
void HurtboxComponent::DebugDrawAndFlush()
{
    if (!s_pProgram) return;
    
    glm::mat4 model = glm::mat4(1.0f);
    s_pProgram->SetUniform("model", model);
    s_pProgram->SetUniform("colour", glm::vec4(0.0f, 0.7f, 0.4f, 0.0f));
    s_pProgram->Bind();
    s_pDecl->Bind();
    
    s_pVB->Bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * s_vVerticesVector.size(), s_vVerticesVector.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINES, 0, s_vVerticesVector.size());

    s_vVerticesVector.clear();
}

int HurtboxComponent::GetComponentCount()
{
    return HurtboxComponent::s_iComponentCount;
}