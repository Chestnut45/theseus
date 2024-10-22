#pragma once

#include <yaml-cpp/yaml.h>

#include "FlatAmtItem.h"
#include "PercentItem.h"
#include "StatusEffectItem.h"
#include "WeaponItem.h"
#include "ArmourItem.h"

//-----------------------------------------------------------------------------
// File:            ItemCreator.h
// Original Author: Aurora Ryder
//
// This namespace and static method lets the user create an item of any type
// by searching a .yaml directory for an entry with a given name
//-----------------------------------------------------------------------------


namespace ItemCreator {
    // The item directory that describes all of the items (index by names)
    const std::string ITEM_DIRECTORY_PATH = "data/item_directory.yaml";

    inline ItemBase* CreateItem(const std::string& p_strItemName) {
        ItemBase* pCreatedItem = nullptr;

        try {
            // First load up the directory
            YAML::Node directory = YAML::LoadFile(ITEM_DIRECTORY_PATH);

            // Then find the entry that matches the name we were given
            YAML::Node itemEntry = directory[p_strItemName];

            // From there, find all of the "common" attributes that all items have
            std::string strItemId = itemEntry["id"] ? itemEntry["id"].as<std::string>() : strItemId;
            std::string strDesc = itemEntry["description"] ? itemEntry["description"].as<std::string>() : strDesc;
            int iValue = itemEntry["value"].as<int>();
            int iTextureFrameIndex = itemEntry["texture_frame"].as<int>();

            // Then figure out what kind of item it is
            if (strItemId == "GOLD") { // If this is a gold item
                // Then we don't need anything else so we can just use an ItemBase to create the item
                pCreatedItem = new ItemBase(GOLD, p_strItemName, strDesc, iValue, true, iTextureFrameIndex);
            }
            else if (strItemId == "CONSUMABLE") { // If it is a consumable
                // All consumables will have these attributes so we look for them
                std::string strType = itemEntry["type"] ? itemEntry["type"].as<std::string>() : strType;
                bool bStackable = itemEntry["is_stackable"] ? itemEntry["is_stackable"].as<bool>() : bStackable;
                int iNumUses = itemEntry["num_uses"].as<int>();

                // Then we figure out what type of consumable item this is so we can find the attributes specific to that type
                if (strType == "FLAT_AMT") {
                    // Flat Amount items have a target attribute and an amount
                    std::string strAttribute = itemEntry["attribute"] ? itemEntry["attribute"].as<std::string>() : strAttribute;
                    float fAmount = itemEntry["amount"].as<float>();

                    // Once we know what attribute it being effected, we need to convert it to the enum equivalent
                    Attribute enTargetAttribute;
                    if (strAttribute == "HEALTH") {
                        enTargetAttribute = HEALTH;
                    }
                    else if (strAttribute == "STAMINA") {
                        enTargetAttribute = STAMINA;
                    }
                    else {
                        // And if we're trying to target an attribute that does not have an enum representation then we can't create the item
                        wolf::Error("ItemCreator Error: Invalid target attribute ", strAttribute.c_str(), " for ", p_strItemName.c_str());
                        return nullptr;
                    }

                    // We now have everything we need to create and return the item, so we do so
                    pCreatedItem = new FlatAmtItem(CONSUMABLE, p_strItemName, strDesc, iValue, bStackable, iTextureFrameIndex, iNumUses, enTargetAttribute, fAmount);

                }
                else if (strType == "PERCENT_AMT") {
                    // Percent Amount items also have a target attribute and an amount
                    std::string strAttribute = itemEntry["attribute"] ? itemEntry["attribute"].as<std::string>() : strAttribute;
                    float fAmount = itemEntry["amount"].as<float>();

                    // Once we know what attribute it being effected, we need to convert it to the enum equivalent
                    Attribute enTargetAttribute;
                    if (strAttribute == "HEALTH") {
                        enTargetAttribute = HEALTH;
                    }
                    else if (strAttribute == "STAMINA") {
                        enTargetAttribute = STAMINA;
                    }
                    else {
                        // And if we're trying to target an attribute that does not have an enum representation then we can't create the item
                        wolf::Error("ItemCreator Error: Invalid target attribute ", strAttribute.c_str(), " for ", p_strItemName.c_str());
                        return nullptr;
                    }

                    // We now have everything we need to create and return the item, so we do so
                    pCreatedItem = new PercentItem(CONSUMABLE, p_strItemName, strDesc, iValue, bStackable, iTextureFrameIndex, iNumUses, enTargetAttribute, fAmount);
                }
                else if (strType == "STATUS_EFFECT") {
                    // Status Effect items have a target status effect and a duration
                    std::string strStatusEffect = itemEntry["status_effect_type"] ? itemEntry["status_effect_type"].as<std::string>() : strStatusEffect;
                    float fDuration = itemEntry["duration"].as<float>();

                    // Once we know which status effect this item causes, we need to convert it to the enum equivalent
                    StatusComponent::StatusEffectType enStatusEffectType;
                    if (strStatusEffect == "BURNING") {
                        enStatusEffectType = StatusComponent::BURNING;
                    }
                    else if (strStatusEffect == "PETRIFIED") {
                        enStatusEffectType = StatusComponent::PETRIFIED;
                    }
                    else if (strStatusEffect == "POISONED") {
                        enStatusEffectType = StatusComponent::POISONED;
                    }
                    else {
                        // If we're trying to apply a status effect that doesn't exist then we can't create the item
                        wolf::Error("ItemCreator Error: Invalid status effect type ", strStatusEffect.c_str(), " for ", p_strItemName.c_str());
                        return nullptr;
                    }

                    // We now have everything we need to create and return the item, so we do so
                    pCreatedItem = new StatusEffectItem(CONSUMABLE, p_strItemName, strDesc, iValue, bStackable, iTextureFrameIndex, iNumUses, enStatusEffectType, fDuration);
                }
                else {
                    // If the item type does not match any of the consumable item enums that we use then we can't create the item
                    return nullptr;
                }
            }
            else if (strItemId == "EQUIPMENT") { // If it is a piece of equipment
                // Figure out which slot it equips into
                std::string strEquipmentSlot = itemEntry["slot"].as<std::string>();

                if (strEquipmentSlot == "WEAPON") { // If this is a weapon
                    // Then we need to know what type of weapon it is and convert it to its enum equivalent
                    std::string strWeaponType = itemEntry["type"].as<std::string>();
                    WeaponType enWeaponType;

                    if (strWeaponType == "SWORD") { // If this is a sword
                        enWeaponType = SWORD;
                    }
                    else if (strWeaponType == "SPEAR") { // If this is a spear
                        enWeaponType = SPEAR;
                    }
                    else if (strWeaponType == "BOW") { // If this is a bow
                        enWeaponType = BOW;
                    }
                    else {
                        // If the string version of the weapon type does not have a corresponding enum then we can't create the item
                        wolf::Error("ItemCreator Error: Invalid weapon type ", strWeaponType.c_str(), " for ", p_strItemName.c_str());
                        return nullptr;
                    }
                    
                    // Then we need to get the properties that all weapons have
                    float fDelay = itemEntry["delay"].as<float>(); // Time between attacks
                    float fDamage = itemEntry["damage"].as<float>(); // How much damage the weapon does

                    // And the size of the weapon's hurtbox (Note that this is different than the projectile hurtbox)
                    glm::vec2 v2HurtboxSize;
                    v2HurtboxSize.x = itemEntry["hurtbox_size"]["x"].as<float>();
                    v2HurtboxSize.y = itemEntry["hurtbox_size"]["y"].as<float>();

                    // Then we need to know if the weapon has projectiles
                    bool bHasProjectiles = itemEntry["has_projectiles"].as<bool>();

                    // Once we have all that, we can create the item
                    WeaponItem* pWeapon = new WeaponItem(EQUIPMENT, p_strItemName, strDesc, iValue, iTextureFrameIndex, enWeaponType, fDelay, fDamage, v2HurtboxSize, bHasProjectiles);
                    
                    // If the weapon has projectiles
                    if (bHasProjectiles) {
                        // We need to find the properties associated with them
                        float fProjectileDamage = itemEntry["projectile"]["damage"].as<float>(); // How much damage each projectile does

                        // How big is the projectile's hurtbox
                        glm::vec2 v2ProjectileHurtbox;
                        v2ProjectileHurtbox.x = itemEntry["projectile"]["hurtbox_size"]["x"].as<float>();
                        v2ProjectileHurtbox.y = itemEntry["projectile"]["hurtbox_size"]["y"].as<float>();

                        // What is the projectile's initial velocity
                        glm::vec2 v2ProjectileVelocity;
                        v2ProjectileVelocity.x = itemEntry["projectile"]["initial_velocity"]["x"].as<float>();
                        v2ProjectileVelocity.y = itemEntry["projectile"]["initial_velocity"]["y"].as<float>();
                        
                        // Where is the projectile's sprite located?
                        std::string strPathToProjectileSprite = itemEntry["projectile"]["sprite_path"].as<std::string>();

                        // Once we have all that, we can attach the properties to the weapon
                        pWeapon->SetProjectileProperties(fProjectileDamage, v2ProjectileHurtbox, v2ProjectileVelocity, strPathToProjectileSprite);
                    }

                    // Then we return the weapon we created
                    pCreatedItem = pWeapon;
                }
                else { // If this is a piece of armour
                    // We need to figure out which armour slot it equips into and convert that to the enum equivalent
                    EquipmentSlot enSlot;

                    if (strEquipmentSlot == "HEAD") { // Equips in the head slot
                        enSlot = HEAD;
                    }
                    else if (strEquipmentSlot == "BODY") { // Equips in the body slot
                        enSlot = BODY;
                    }
                    else if (strEquipmentSlot == "ARMS") { // Equips in the arms slot
                        enSlot = ARMS;
                    }
                    else if (strEquipmentSlot == "LEGS") { // Equips in the legs slot
                        enSlot = LEGS;
                    }
                    else if (strEquipmentSlot == "FEET") { // Equips in the feet slot
                        enSlot = FEET;
                    }
                    else if (strEquipmentSlot == "GLOVES") { // Equips in the gloves slot
                        enSlot = GLOVES;
                    }
                    else if (strEquipmentSlot == "ACCESSORY") { // Equips in the accessory slot
                        enSlot = ACCESSORY;
                    }
                    else {
                        // If there is not an enum equivalent to the string equipment slot then we cannot create the item
                        wolf::Log("ItemCreator Error: Invalid equipment slot ", strEquipmentSlot.c_str(), " for ", p_strItemName.c_str());
                        return nullptr;
                    }

                    // How much damage does this armour piece reduce (as a percentage)
                    float fDamageReduction = itemEntry["damage_reduction"].as<float>();

                    // Does this armour piece apply any status effects?
                    bool bHasStatusEffects = itemEntry["has_status_effects"].as<bool>();
                    std::vector<ArmourStatusEffect> vStatusEffects;

                    // If it does...
                    if (bHasStatusEffects) {
                        // Get the list of status effects
                        YAML::Node effectList = itemEntry["status_effects"];

                        // Then iterate through it
                        for (int j = 0; j < effectList.size(); ++j) {
                            // Figure out what type of status effect this is
                            std::string strEffectType = effectList[j]["type"].as<std::string>();

                            // And convert it to the enum equivalent
                            StatusComponent::StatusEffectType enEffectType;
                            if (strEffectType == "BURNING") {
                                enEffectType = StatusComponent::BURNING;
                            }
                            else if (strEffectType == "PETRIFIED") {
                                enEffectType = StatusComponent::PETRIFIED;
                            }
                            else if (strEffectType == "POISONED") {
                                enEffectType = StatusComponent::POISONED;
                            }
                            else {
                                // If we're trying to apply a status effect that doesn't exist then we can't create the item
                                wolf::Error("ItemCreator Error: Invalid status effect type ", strEffectType.c_str(), " for ", p_strItemName.c_str());
                                return nullptr;
                            }

                            // Then find the effect's duration
                            float fDuration = effectList[j]["duration"].as<float>();

                            // And add the effect to the vector
                            vStatusEffects.push_back({enEffectType, fDuration});
                        }
                    }

                    // Then we can create the armour item!
                    pCreatedItem = new ArmourItem(EQUIPMENT, p_strItemName, strDesc, iValue, iTextureFrameIndex, enSlot, fDamageReduction, vStatusEffects);

                }
            }
            else {
                // If the ID doesn't match one of the enums that we use then we can't create the item
                wolf::Error("ItemCreator Error: Invalid item type id ", strItemId.c_str(), " for ", p_strItemName.c_str());
                return nullptr;
            }
        }
        catch(YAML::Exception& e) {
            wolf::Error("Error using '", ITEM_DIRECTORY_PATH.c_str(), "to create ", p_strItemName.c_str(), ": ", e.what());
            return nullptr;
        }

        return pCreatedItem;
    }
}