#pragma once

#include <wolf.h>
//-----------------------------------------------------------------------------
// File: SharedContext.h
// Original Author: Youssef Ashraf
// stores entity id's to be shared between states.
//-----------------------------------------------------------------------------
class SharedContext {
public:
    void RegisterEntity(const std::string& name, wolf::GameObjectID id) {
        m_entityIDs[name] = id;
    }

    wolf::GameObjectID GetEntityID(const std::string& name) const {
        auto it = m_entityIDs.find(name);
        if (it != m_entityIDs.end()) {
            return it->second;
        }
        wolf::Error("Entity ID not found for: " + name);
        return static_cast<wolf::GameObjectID>(-1);
    }

    bool HasEntity(const std::string& name) const {
        return m_entityIDs.find(name) != m_entityIDs.end();
    }

    const std::unordered_map<std::string, wolf::GameObjectID>& GetEntityIDs() const {
        return m_entityIDs;
    }

private:
    std::unordered_map<std::string, wolf::GameObjectID> m_entityIDs;
};
 
