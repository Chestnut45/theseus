#include "VelocityManager.h"

VelocityManager::VelocityManager(wolf::Scene* p_scene)
{
    this->m_scene = p_scene;
}

VelocityManager::~VelocityManager()
{
    this->m_scene = nullptr;
}

void VelocityManager::Update(float p_delta)
{
    if(VelocityComponent::s_iComponentCount >= 1)
    {
        for (auto&&[id, object, velocity] : this->m_scene->Each<wolf::GameObject, VelocityComponent>())
        {
            if(velocity.m_fKnockbackForce > 0.0f)
            {   
                //std::cout << "VelocityManager - id: " << id << " - KBForce: " << velocity.m_fKnockbackForce << std::endl;
                velocity.m_fKnockbackForce -= 0.05f;
            }
            else{
                velocity.m_fKnockbackForce = 0.0f;
            }
        }
    }
}