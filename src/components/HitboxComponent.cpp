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

const std::vector<Vertex2D> uniqueVertices = 
{
    {0.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 1.0f},
    {1.0f, 0.0f}
};

int HitboxComponent::s_iComponentCount = 0;

std::vector<Vertex2D> HitboxComponent::s_vVerticesVector;

wolf::VertexDeclaration * HitboxComponent::s_pDecl = nullptr;
wolf::Program *HitboxComponent::s_pProgram = nullptr;
wolf::VertexBuffer *HitboxComponent::s_pVB = nullptr;

// HitboxComponent::HitboxComponent():
// m_Hitbox(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f))
// {


//     HitboxComponent::s_iComponentCount++;
//     if (s_pProgram == nullptr)
//     {
//         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

//         s_pProgram = wolf::ProgramManager::CreateProgram("data/shaders/lines.vsh", "data/shaders/lines.fsh");
//         s_pVB = wolf::BufferManager::CreateVertexBuffer(vertices.data(), sizeof(Vertex2D) * 8);

//         s_pDecl = new wolf::VertexDeclaration();
//         s_pDecl->Begin();
//         s_pDecl->AppendAttribute(wolf::AT_Position, 2, wolf::CT_Float);
//         s_pDecl->SetVertexBuffer(s_pVB);
//         s_pDecl->End();
//     }
// }

// Constructor for custom attributes
HitboxComponent::HitboxComponent(bool p_doc, bool p_relativity)
{
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
    HitboxComponent::s_iComponentCount++;
    HitboxManager::s_iComponentCount++;
}

// Destructor
HitboxComponent::~HitboxComponent()
{   
    HitboxComponent::s_iComponentCount--;
    HitboxManager::s_iComponentCount--;
    if(HitboxComponent::s_iComponentCount == 0)
    {
        delete HitboxComponent::s_pDecl;
        HitboxComponent::s_pDecl = nullptr;
        wolf::ProgramManager::DestroyProgram(HitboxComponent::s_pProgram);
        HitboxComponent::s_pProgram = nullptr;
        wolf::BufferManager::DestroyBuffer(HitboxComponent::s_pVB);
        HitboxComponent::s_pVB = nullptr;
    }
}

// Add hitbox to vector
void HitboxComponent::AddHitbox(glm::vec2 p_dimensions)
{
    this->m_vHitboxes.push_back(wolf::Rectangle(glm::vec2(0.0f, 0.0f), p_dimensions));
}

void HitboxComponent::AddHitbox(glm::vec2 p_dimensions, glm::vec2 p_offset)
{
    this->m_vHitboxes.push_back(wolf::Rectangle(p_offset, p_dimensions));
}

// Get vector of hitboxes
std::vector<wolf::Rectangle> HitboxComponent::GetHitboxes() const
{
    return this->m_vHitboxes;
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
    glm::vec2 translation = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    for(wolf::Rectangle hitbox : this->m_vHitboxes)
    {
        glm::vec2 dimensions = glm::vec2(hitbox.GetWidth(), hitbox.GetHeight());
        glm::vec2 offset = hitbox.GetPosition();
        
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

void HitboxComponent::DebugDrawAndFlush()
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

    //std::cout << "Count: " << HitboxComponent::s_iComponentCount << std::endl;
    for(Vertex2D vertex: s_vVerticesVector)
    {
        //std::cout << "X: " << vertex.x << ", Y: " << vertex.y << std::endl;
    }
    s_vVerticesVector.clear();
}

int HitboxComponent::GetComponentCount()
{
    return HitboxComponent::s_iComponentCount;
}