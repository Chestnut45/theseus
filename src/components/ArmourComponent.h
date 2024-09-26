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
    enum SpecialProperty
    {
        CONTACTDAMAGE,
        FIRERESISTANCE,
        HEALING,
        NONE
    };

    ArmourComponent();
    ~ArmourComponent();

    void CollectArmour(int p_multiplier, std::initializer_list<std::pair<SpecialProperty, float>> p_special_properties_info = {{SpecialProperty::NONE, 0}});
    bool IsSpecialPropertyPresent(SpecialProperty p_special_property) const;
    float GetSpecialPropertiesValues(SpecialProperty p_special_property) const;
    int GetMultiplier() const;

private:
    int m_iMultiplier = 0; // Default multiplier
    bool m_aSpecialProperties[SpecialProperty::NONE]; // Flags for special properties of current armour
    float m_aSpecialPropertiesValues[SpecialProperty::NONE]; // Values for special properties of current armour (if applicable)

};