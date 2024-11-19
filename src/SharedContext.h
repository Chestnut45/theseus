#include "wolf.h"
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
        throw std::runtime_error("Entity ID not found for: " + name);
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
