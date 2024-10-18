#include "EnemyDataLoader.h"

std::vector<EnemyData> EnemyDataLoader::LoadAllEnemyData(const std::string& filepath) {
    YAML::Node node = YAML::LoadFile(filepath);
    YAML::Node enemiesNode = node["enemies"];

    std::vector<EnemyData> enemies;
    for (std::size_t i = 0; i < enemiesNode.size(); ++i) {
        YAML::Node enemyNode = enemiesNode[i];

        EnemyData data;
        data.type = enemyNode["type"].as<std::string>();
        data.health = enemyNode["health"].as<int>();
        data.armour = enemyNode["armour"].as<int>();
        data.position = glm::vec2(enemyNode["position"]["x"].as<float>(), enemyNode["position"]["y"].as<float>());
        data.scale = enemyNode["scale"].as<float>();
        data.meleeRange = enemyNode["melee_range"].as<float>();
        data.attackCooldown = enemyNode["attack_cooldown"].as<float>();
        data.detectionRange = enemyNode["detection_range"].as<float>();
        data.baseDamage = enemyNode["base_damage"].as<float>();
        data.chaseSpeed = enemyNode["chase_speed"].as<float>();
        data.animationSheet = enemyNode["animation_sheet"].as<std::string>();

        enemies.push_back(data);
    }
    return enemies;
}