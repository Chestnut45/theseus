#pragma once
#include <string>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

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
    std::vector<EnemyData> LoadAllEnemyData(const std::string& filepath);
};
