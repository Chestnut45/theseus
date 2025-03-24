#pragma once
//-----------------------------------------------------------------------------
// File: Blackboard.h
// Original Author: Youssef Ashraf
// Simple blackboard implementation for sharing data between AI nodes
//-----------------------------------------------------------------------------

#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <glm/vec2.hpp>

// Attack strategies
enum class Strategy { AGGRESSIVE, DEFENSIVE, FLANKING };

class Blackboard {
public:
    // Basic data storage
    void SetBool(const std::string& key, bool value) {
        m_boolData[key] = value;
    }
    
    bool GetBool(const std::string& key) const {
        auto it = m_boolData.find(key);
        return (it != m_boolData.end()) ? it->second : false;
    }
    
    void SetFloat(const std::string& key, float value) {
        m_floatData[key] = value;
    }
    
    float GetFloat(const std::string& key) const {
        auto it = m_floatData.find(key);
        return (it != m_floatData.end()) ? it->second : 0.0f;
    }
    
    void SetInt(const std::string& key, int value) {
        m_intData[key] = value;
    }
    
    int GetInt(const std::string& key) const {
        auto it = m_intData.find(key);
        return (it != m_intData.end()) ? it->second : 0;
    }

    void SetVector2(const std::string& key, const glm::vec2& value) {
        m_vec2Data[key] = value;
    }

    glm::vec2 GetVector2(const std::string& key) const {
        auto it = m_vec2Data.find(key);
        return (it != m_vec2Data.end()) ? it->second : glm::vec2(0.0f);
    }
    
    // Strategy management
    void SetStrategy(Strategy strategy) {
        m_strategy = strategy;
    }
    
    Strategy GetStrategy() const {
        return m_strategy;
    }
    
    // Attacking enemy coordination
    void SetAttackingEnemyID(int id) {
        m_attackingEnemyID = id;
    }
    
    int GetAttackingEnemyID() const {
        return m_attackingEnemyID;
    }

    // Register an enemy in the group
    void RegisterEnemy(int enemyID) {
        if (std::find(m_enemyIDs.begin(), m_enemyIDs.end(), enemyID) == m_enemyIDs.end()) {
            m_enemyIDs.push_back(enemyID);
        }
    }

    // Remove an enemy from the group
    void UnregisterEnemy(int enemyID) {
        auto it = std::find(m_enemyIDs.begin(), m_enemyIDs.end(), enemyID);
        if (it != m_enemyIDs.end()) {
            m_enemyIDs.erase(it);
        }

        // If this was the attacking enemy, reset it
        if (m_attackingEnemyID == enemyID) {
            m_attackingEnemyID = -1;
        }
    }

    // Get all enemies in the current group
    const std::vector<int>& GetEnemyGroup() const {
        return m_enemyIDs;
    }
    
    // Clear all data
    void Clear() {
        m_boolData.clear();
        m_floatData.clear();
        m_intData.clear();
        m_vec2Data.clear();
        m_strategy = Strategy::AGGRESSIVE;
        m_attackingEnemyID = -1;
        m_enemyIDs.clear();
    }

private:
    std::unordered_map<std::string, bool> m_boolData;
    std::unordered_map<std::string, float> m_floatData;
    std::unordered_map<std::string, int> m_intData;
    std::unordered_map<std::string, glm::vec2> m_vec2Data;
    Strategy m_strategy = Strategy::AGGRESSIVE;
    int m_attackingEnemyID = -1;
    std::vector<int> m_enemyIDs; // IDs of enemies in the current group
};

// Create a global blackboard instance
extern Blackboard g_blackboard;