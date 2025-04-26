//-----------------------------------------------------------------------------
// File: EnemyDataLoader.h
// Original Author:	Youssef Ashraf
// Modifications : Nguyễn Minh Nhật
// Loads data for enemies from .yaml files.
//-----------------------------------------------------------------------------
#pragma once
#include <string>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>
#include "W_Logging.h"

struct EnemyData 
{
    std::string type = "";
    int health = 0;
    int armour = 0;
    glm::vec2 position{0.0f};
    float scale = 1.0f;
    float meleeRange = 1.0f;
    float rangedRange = -1.0f;
    float attackCooldown = 1.0f;
    float meleeCooldown = 1.0f; // Delay between melee attacks
    float rangedCooldown = 1.0f; // Delay between ranged attacks
    float meleeWindup = 1.0f; // Melee windup time
    float rangedWindup = 1.0f; // Ranged windup time
    float detectionRange = 1.0f;
    float baseDamage = 0.0f;
    float chaseSpeed = 1.0f;
    std::string animationInitFile = "";
};

class EnemyDataLoader {

public:

    // Loads the enemy data from the given yaml filepath
    // NOTE: If file is already cached, will not load from disk again unless reload is true
    static void LoadAllEnemyData(const std::string& filepath, bool reloadFromDisk = false);

    // Loads a specific enemy's cached data after LoadAllEnemyData has been called
    static EnemyData LoadEnemyData(const std::string& type);

private:
    static inline std::unordered_map<std::string, EnemyData> s_enemyCache;

    // Map of filepaths to cached config nodes
    static inline std::unordered_map<std::string, YAML::Node> s_configCache;

    static inline std::string s_lastLoadedConfig;
};
