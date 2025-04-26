//-----------------------------------------------------------------------------
// File: EnemyDataLoader.cpp
// Original Author:	Youssef Ashraf
// Modifications : Nguyễn Minh Nhật
// Loads data for enemies from .yaml files.
//-----------------------------------------------------------------------------
#include "EnemyDataLoader.h"

void EnemyDataLoader::LoadAllEnemyData(const std::string& filepath, bool reloadFromDisk) {

    try
    {
        YAML::Node node;
        if (s_configCache.contains(filepath) && !reloadFromDisk)
        {
            // Early out if this config was already loaded last
            if (s_lastLoadedConfig == filepath) return;
            node = s_configCache[filepath];
        }
        else
        {
            // Load from disk
            node = YAML::LoadFile(filepath);
            s_configCache[filepath] = node;
        }

        // Update last loaded config
        s_lastLoadedConfig = filepath;
        
        YAML::Node enemiesNode = node["enemies"];

        for (std::size_t i = 0; i < enemiesNode.size(); ++i) {
            YAML::Node enemyNode = enemiesNode[i];

            EnemyData data;
            data.type = enemyNode["type"].as<std::string>();
            data.health = enemyNode["health"].as<int>();
            data.armour = enemyNode["armour"].as<int>();
            data.meleeRange = enemyNode["melee_range"] ? enemyNode["melee_range"].as<float>() : 1.0f;
            data.rangedRange = enemyNode["ranged_range"] ? enemyNode["ranged_range"].as<float>() : 1.0f;
            data.meleeCooldown = enemyNode["melee_cooldown"] ? enemyNode["melee_cooldown"].as<float>() : 1.0f;
            data.rangedCooldown = enemyNode["ranged_cooldown"] ? enemyNode["ranged_cooldown"].as<float>() : 1.0f;
            data.meleeWindup = enemyNode["melee_windup"] ? enemyNode["melee_windup"].as<float>() : 1.0f;
            data.rangedWindup = enemyNode["ranged_windup"] ? enemyNode["ranged_windup"].as<float>() : 1.0f;
            data.detectionRange = enemyNode["detection_range"].as<float>();
            data.baseDamage = enemyNode["base_damage"].as<float>();
            data.chaseSpeed = enemyNode["chase_speed"].as<float>();
            data.animationInitFile = enemyNode["animation_init_file"].as<std::string>();

            // Store the data in the map with the type as the key
            s_enemyCache[data.type] = data;

            // std::cout << "EnemyDataLoader - type: " << data.type << std::endl;
        }
    }
    catch (YAML::Exception& e)
    {
        wolf::Error("Error parsing file '", filepath.c_str(), "': ", e.what());
    }
}

EnemyData EnemyDataLoader::LoadEnemyData(const std::string& type) {
    if (s_enemyCache.find(type) != s_enemyCache.end()) {
        return s_enemyCache[type];
    }

    // Log an error message using wolf's error logging system and return a default EnemyData
    wolf::Error("EnemyDataLoader: Enemy type '", type.c_str(), "' not found.");
    
    // Return a default/empty EnemyData object to avoid runtime crashes
    return EnemyData();
}