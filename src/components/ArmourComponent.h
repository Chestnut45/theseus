//-----------------------------------------------------------------------------
// File: ArmourComponent.h
// Original Author: Nguyễn Minh Nhật
// Armour.
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class ArmourComponent: public wolf::BaseComponent
{
public:
    enum ArmourSpecialProperty
    {
        CONTACTDAMAGE,
        HEALING,
        NONE
    };

    ArmourComponent(int p_multiplier);
    ~ArmourComponent();

    void CollectArmour(int p_multiplier, std::vector<ArmourSpecialProperty> p_special_properties);
    bool IsSpecialPropertyPresent(ArmourSpecialProperty p_special_property) const;
    int GetMultiplier() const;

private:
    int m_iMultiplier = 0; // Default multiplier
    bool m_aSpecialProperties[ArmourSpecialProperty::NONE];

};