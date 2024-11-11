#include "EnemyDataLoader.h"

void EnemyDataLoader::LoadAllEnemyData(const std::string& filepath) {

    try
    {
        YAML::Node node = YAML::LoadFile(filepath);
        YAML::Node enemiesNode = node["enemies"];

        for (std::size_t i = 0; i < enemiesNode.size(); ++i) {
            YAML::Node enemyNode = enemiesNode[i];

            EnemyData data;
            data.type = enemyNode["type"].as<std::string>();
            data.health = enemyNode["health"].as<int>();
            data.armour = enemyNode["armour"].as<int>();
            data.meleeRange = enemyNode["melee_range"].as<float>();
            data.attackCooldown = enemyNode["attack_cooldown"].as<float>();
            data.detectionRange = enemyNode["detection_range"].as<float>();
            data.baseDamage = enemyNode["base_damage"].as<float>();
            data.chaseSpeed = enemyNode["chase_speed"].as<float>();
            data.animationInitFile = enemyNode["animation_init_file"].as<std::string>();

            // Store the data in the map with the type as the key
            m_enemyCache[data.type] = data;

            std::cout << "EnemyDataLoader - type: " << data.type << std::endl;
        }
    }
    catch (YAML::Exception& e)
    {
        wolf::Error("Error parsing file '", filepath.c_str(), "': ", e.what());
    }
}

EnemyData EnemyDataLoader::LoadEnemyData(const std::string& type) {
    if (m_enemyCache.find(type) != m_enemyCache.end()) {
        return m_enemyCache[type];
    }

    // Log an error message using wolf's error logging system and return a default EnemyData
    wolf::Error("EnemyDataLoader: Enemy type '", type.c_str(), "' not found.");
    
    // Return a default/empty EnemyData object to avoid runtime crashes
    return EnemyData();
}