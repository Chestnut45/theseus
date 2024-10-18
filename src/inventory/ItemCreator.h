#pragma once

#include <yaml-cpp/yaml.h>

#include "FlatAmtItem.h"
#include "PercentItem.h"
#include "StatusEffectItem.h"
#include "EquipmentItem.h"

namespace ItemCreator {
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
            if (strItemId == "CONSUMABLE") { // If it is a consumable
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

            }
            else {
                // If the ID doesn't match one of the enums that we use then we can't create the item
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