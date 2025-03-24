#pragma once
//-----------------------------------------------------------------------------
// File: BehaviorTree.h
// Original Author: Youssef Ashraf
// Minimal behavior tree implementation for Theseus enemies
//-----------------------------------------------------------------------------

#include <vector>
#include <memory>
#include <string>
#include <functional>

// Forward declaration
class Blackboard;

// Base class for all behavior tree nodes
class BehaviorNode {
public:
    enum class Status { SUCCESS, FAILURE, RUNNING };
    
    virtual ~BehaviorNode() = default;
    virtual Status Execute(float deltaTime, Blackboard* blackboard) = 0;
};

// Selector: Succeeds if ANY child succeeds, fails if ALL fail
class Selector : public BehaviorNode {
public:
    Status Execute(float deltaTime, Blackboard* blackboard) override {
        for (auto& child : m_children) {
            Status status = child->Execute(deltaTime, blackboard);
            if (status != Status::FAILURE) {
                return status; // Return SUCCESS or RUNNING
            }
        }
        return Status::FAILURE;
    }
    
    void AddBehaviorNode(std::unique_ptr<BehaviorNode> child) {
        m_children.push_back(std::move(child));
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> m_children;
};

// Sequence: Fails if ANY child fails, succeeds if ALL succeed
class Sequence : public BehaviorNode {
public:
    Status Execute(float deltaTime, Blackboard* blackboard) override {
        for (auto& child : m_children) {
            Status status = child->Execute(deltaTime, blackboard);
            if (status != Status::SUCCESS) {
                return status; // Return FAILURE or RUNNING
            }
        }
        return Status::SUCCESS;
    }
    
    void AddBehaviorNode(std::unique_ptr<BehaviorNode> child) {
        m_children.push_back(std::move(child));
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> m_children;
};

// Condition node - returns SUCCESS or FAILURE based on a condition
class ConditionNode : public BehaviorNode {
public:
    using ConditionFunction = std::function<bool()>;
    
    explicit ConditionNode(ConditionFunction condition) 
        : m_condition(condition) {}
    
    Status Execute(float deltaTime, Blackboard* blackboard) override {
        return m_condition() ? Status::SUCCESS : Status::FAILURE;
    }

private:
    ConditionFunction m_condition;
};

// Action node - performs an action and returns a status
class ActionNode : public BehaviorNode {
public:
    using ActionFunction = std::function<Status(float)>;
    
    explicit ActionNode(ActionFunction action) 
        : m_action(action) {}
    
    Status Execute(float deltaTime, Blackboard* blackboard) override {
        return m_action(deltaTime);
    }

private:
    ActionFunction m_action;
};

// Simple behavior tree
class BehaviorTree {
public:
    explicit BehaviorTree(std::unique_ptr<BehaviorNode> rootNode)
        : m_root(std::move(rootNode)) {}
    
    void Update(float deltaTime, Blackboard* blackboard) {
        if (m_root) {
            m_root->Execute(deltaTime, blackboard);
        }
    }

private:
    std::unique_ptr<BehaviorNode> m_root;
};