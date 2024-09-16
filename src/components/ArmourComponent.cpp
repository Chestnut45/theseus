//-----------------------------------------------------------------------------
// File: ArmourComponent.cpp
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Armour.
//-----------------------------------------------------------------------------

#include "ArmourComponent.h"

int ArmourComponent::GetMultiplier() const
{
    return this->m_multiplier;
}

void ArmourComponent::SetMultiplier(int p_multiplier)
{
    this->m_multiplier = p_multiplier;
}