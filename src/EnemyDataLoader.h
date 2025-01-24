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
    void LoadAllEnemyData(const std::string& filepath);
    EnemyData LoadEnemyData(const std::string& type);
private:
    std::unordered_map<std::string, EnemyData> m_enemyCache;
};
