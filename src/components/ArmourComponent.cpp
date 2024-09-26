//-----------------------------------------------------------------------------
// File: ArmourComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Armour.
//-----------------------------------------------------------------------------

#include "ArmourComponent.h"

ArmourComponent::ArmourComponent(int p_multiplier)
{
    this->m_iMultiplier = p_multiplier;
    for(int i = 0; i < ArmourSpecialProperty::NONE; i++)
    {
        m_aSpecialProperties[i] = 0;
    }
}

ArmourComponent::~ArmourComponent()
{

}

void ArmourComponent::CollectArmour(int p_multiplier, std::vector<ArmourSpecialProperty> p_special_properties)
{
    this->m_iMultiplier = p_multiplier;
    
    // Set all special property flags to 0
    for(int i = 0; i < ArmourSpecialProperty::NONE; i++)
    {
        m_aSpecialProperties[i] = 0;
    }

    // Set provided special property flags to 1
    for(ArmourSpecialProperty specialProperty: p_special_properties)
    {
        this->m_aSpecialProperties[specialProperty] = 1;
    }
}

bool ArmourComponent::IsSpecialPropertyPresent(ArmourSpecialProperty p_special_property) const
{
    return this->m_aSpecialProperties[p_special_property];
}

int ArmourComponent::GetMultiplier() const
{
    return this->m_iMultiplier;
}