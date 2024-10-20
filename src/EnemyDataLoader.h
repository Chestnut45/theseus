#pragma once
#include <string>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>
#include "W_Logging.h"

struct EnemyData 
{
    std::string type;
    int health;
    int armour;
    glm::vec2 position;
    float scale;
    float meleeRange;
    float attackCooldown;
    float detectionRange;
    float baseDamage;
    float chaseSpeed;
    std::string animationSheet;
};
class EnemyDataLoader {

public:
    void LoadAllEnemyData(const std::string& filepath);
    EnemyData LoadEnemyData(const std::string& type);
private:
    std::unordered_map<std::string, EnemyData> m_enemyCache;
};
