//-----------------------------------------------------------------------------
// File: ColliderComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Collider.
//-----------------------------------------------------------------------------

#include "ColliderComponent.h"

const std::vector<Vertex2D> vertices = 
{
    {0.0f, 0.0f},
    {0.0f, -1.0f},

    {0.0f, -1.0f},
    {1.0f, -1.0f},

    {1.0f, -1.0f},
    {1.0f, 0.0f},

    {1.0f, 0.0f},
    {0.0f, 0.0f}
};

int ColliderComponent::s_iComponentCount = 0;

std::vector<Vertex2D> ColliderComponent::s_vVerticesVector;

wolf::VertexDeclaration * ColliderComponent::s_pDecl = nullptr;
wolf::Program *ColliderComponent::s_pProgram = nullptr;
wolf::VertexBuffer *ColliderComponent::s_pVB = nullptr;

// Constructor for custom attributes
ColliderComponent::ColliderComponent(ColliderType p_collider_type, bool p_doc, bool p_relativity, wolf::GameObjectID p_ignore_id)
{
    this->m_eColliderType = p_collider_type;
    this->m_bIsDestroyedOnCollision = p_doc;
    this->m_bIsRelative = p_relativity;
    this->m_IgnoreID = p_ignore_id;

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
    ColliderComponent::s_iComponentCount++;
}

ColliderComponent::~ColliderComponent()
{
    // std::cout << "ColliderComponent - Delete id: " << this->GetGameObject()->GetID() << std::endl;
    ColliderComponent::s_iComponentCount--;
    if(ColliderComponent::s_iComponentCount == 0)
    {
        delete ColliderComponent::s_pDecl;
        ColliderComponent::s_pDecl = nullptr;
        wolf::ProgramManager::DestroyProgram(ColliderComponent::s_pProgram);
        ColliderComponent::s_pProgram = nullptr;
        wolf::BufferManager::DestroyBuffer(ColliderComponent::s_pVB);
        ColliderComponent::s_pVB = nullptr;
    }
}

void ColliderComponent::AddColliderBox(glm::vec2 p_dimensions)
{
    this->m_vColliderBoxes.push_back(wolf::Rectangle(glm::vec2(0.0f, 0.0f), p_dimensions));
}

void ColliderComponent::AddColliderBox(glm::vec2 p_dimensions, glm::vec2 p_offset)
{
    this->m_vColliderBoxes.push_back(wolf::Rectangle(p_offset, p_dimensions));
}

// Get vector of collider boxes
std::vector<wolf::Rectangle> ColliderComponent::GetColliderBoxes() const
{
    return this->m_vColliderBoxes;
}

bool ColliderComponent::IsHitbox() const
{
    if(this->m_eColliderType == ColliderType::HITBOX || this->m_eColliderType == ColliderType::HITHURTBOXDD || this->m_eColliderType == ColliderType::HITHURTBOXDR)
    {
        return true;
    }
    return false;
}

bool ColliderComponent::IsHurtbox() const
{
    if(this->m_eColliderType == ColliderType::HURTBOXDD || this->m_eColliderType == ColliderType::HURTBOXDR || this->m_eColliderType == ColliderType::HITHURTBOXDD || this->m_eColliderType == ColliderType::HITHURTBOXDR)
    {
        return true;
    }
    return false;
}

bool ColliderComponent::IsHurtboxDamageDealer() const
{
    if(this->m_eColliderType == ColliderType::HURTBOXDD || this->m_eColliderType == ColliderType::HITHURTBOXDD)
    {
        return true;
    }
    return false;
}

bool ColliderComponent::IsHurtboxDamageReceiver() const
{
    if(this->m_eColliderType == ColliderType::HURTBOXDR || this->m_eColliderType == ColliderType::HITHURTBOXDR)
    {
        return true;
    }
    return false;
}

bool ColliderComponent::IsDestroyedOnCollision() const
{
    return this->m_bIsDestroyedOnCollision;
}

// Get relativity
bool ColliderComponent::IsRelative() const
{
    return this->m_bIsRelative;
}
// Set collider type
void ColliderComponent::SetColliderType(ColliderComponent::ColliderType p_collider_type)
{
    this->m_eColliderType = p_collider_type;
}

// Get collider type
ColliderComponent::ColliderType ColliderComponent::GetColliderType() const
{
    return this->m_eColliderType;
}

void ColliderComponent::SetIgnoreTag(wolf::GameObjectID p_id)
{
    this->m_IgnoreID = p_id;
}

// Fill vertex array with vertices of instance
void ColliderComponent::FillVertexArray()
{
    glm::vec2 translation = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    for(wolf::Rectangle colliderBox : this->m_vColliderBoxes)
    {
        glm::vec2 dimensions = glm::vec2(colliderBox.GetWidth(), colliderBox.GetHeight());
        glm::vec2 offset = colliderBox.GetPosition();

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
void ColliderComponent::DebugDrawAndFlush()
{
    if (!s_pProgram) return;
    
    glm::mat4 model = glm::mat4(1.0f);
    s_pProgram->SetUniform("model", model);
    s_pProgram->SetUniform("colour", glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
    s_pProgram->Bind();
    s_pDecl->Bind();
    
    s_pVB->Bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * s_vVerticesVector.size(), s_vVerticesVector.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINES, 0, s_vVerticesVector.size());

    s_vVerticesVector.clear();
}

int ColliderComponent::GetComponentCount()
{
    return ColliderComponent::s_iComponentCount;
}

//setter, sets for m_active
void ColliderComponent::SetActive(bool active) {
    m_active = active;
}

//getter, returns m_active
bool ColliderComponent::IsActive() const {
    return m_active;
}

std::vector<glm::vec2> ColliderComponent::GetWorldSpaceCorners() {
    // Iterate through the boxes that makeup this collider
    glm::vec2 translation = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    std::vector<glm::vec2> vv2CorrectVertices;

    for(wolf::Rectangle colliderBox : this->m_vColliderBoxes)
    {
        glm::vec2 dimensions = glm::vec2(colliderBox.GetWidth(), colliderBox.GetHeight());
        glm::vec2 offset = colliderBox.GetPosition();

        bool bSkip = false;
        for(Vertex2D vertex : vertices)
        {
            if (bSkip) {
                bSkip = false;
            }
            else {
                glm::vec2 v2CorrectVertex;
                if(this->m_bIsRelative)
                {
                    glm::vec2 scale = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                    v2CorrectVertex.x = vertex.x * dimensions.x * scale.x + translation.x + offset.x * scale.x;
                    v2CorrectVertex.y = vertex.y * dimensions.y * scale.y + translation.y + offset.y * scale.y;
                }
                else{
                    v2CorrectVertex.x = vertex.x * dimensions.x + translation.x + offset.x;
                    v2CorrectVertex.y = vertex.y * dimensions.y + translation.y + offset.y;
                }  
                
                vv2CorrectVertices.push_back(v2CorrectVertex);
                bSkip = true;
            }
        }
    }

    return vv2CorrectVertices;
}