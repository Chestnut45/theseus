//-----------------------------------------------------------------------------
// File: ArmourComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Armour.
//-----------------------------------------------------------------------------

#include "ArmourComponent.h"

ArmourComponent::ArmourComponent(int p_multiplier)
{
    this->m_multiplier = p_multiplier;
}

int ArmourComponent::GetMultiplier() const
{
    return this->m_multiplier;
}

void ArmourComponent::SetMultiplier(int p_multiplier)
{
    this->m_multiplier = p_multiplier;
}