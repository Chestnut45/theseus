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

HurtboxComponent::HurtboxComponent()
{
    this->m_pHurtbox = new wolf::Rectangle(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f));
}

// Constructor for custom attributes
HurtboxComponent::HurtboxComponent(glm::vec2 p_dimensions, bool p_type, float p_damage, bool p_doc, bool p_relativity)
{
    this->m_pHurtbox = new wolf::Rectangle(glm::vec2(0.0f, 0.0f), p_dimensions);
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

}

// Get wolf::Rectangle
wolf::Rectangle* HurtboxComponent::GetHurtbox()
{
    return this->m_pHurtbox;
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

// Get dimensions
glm::vec2 HurtboxComponent::GetDimensions() const
{
    glm::vec2 dimensions = glm::vec2(this->m_pHurtbox->GetWidth(), this->m_pHurtbox->GetHeight());
    if(this->m_bIsRelative)
    {
        glm::vec2 scale = GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
        dimensions *= scale;
    }
    return dimensions;
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