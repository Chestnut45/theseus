//-----------------------------------------------------------------------------
// File: ArmourComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Armour.
//-----------------------------------------------------------------------------

#include "ArmourComponent.h"

ArmourComponent::ArmourComponent()
{
    this->m_iMultiplier = 0;
    for(int i = 0; i < SpecialProperty::NONE; i++)
    {
        m_aSpecialProperties[i] = 0;
        m_aSpecialPropertiesValues[i] = 0;
    }
}

ArmourComponent::~ArmourComponent()
{

}

void ArmourComponent::CollectArmour(int p_multiplier, std::initializer_list<std::pair<SpecialProperty, float>> p_special_properties_info)
{
    this->m_iMultiplier = p_multiplier;
    
    // Set all special property flags to 0
    for(int i = 0; i <SpecialProperty::NONE; i++)
    {
        m_aSpecialProperties[i] = 0;
        m_aSpecialPropertiesValues[i] = 0;
    }

    // Set provided special property flags to 1
    for(std::pair<SpecialProperty, float> info: p_special_properties_info)
    {
        SpecialProperty specialProperty = info.first;
        float specialPropertyValue = info.second;

        this->m_aSpecialProperties[specialProperty] = 1;
        this->m_aSpecialPropertiesValues[specialProperty] = specialPropertyValue;
    }
}

bool ArmourComponent::IsSpecialPropertyPresent(SpecialProperty p_special_property) const
{
    return this->m_aSpecialProperties[p_special_property];
}

float ArmourComponent::GetSpecialPropertiesValues(SpecialProperty p_special_property) const
{
    return this->m_aSpecialPropertiesValues[p_special_property];
}

int ArmourComponent::GetMultiplier() const
{
    return this->m_iMultiplier;
}