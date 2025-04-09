//-----------------------------------------------------------------------------
// File: BehaviorTree.cpp
// Original Author: Youssef Ashraf
// Minimal behavior tree implementation for Theseus enemies
//-----------------------------------------------------------------------------
#include "BehaviorTree.h"

// Selector implementation
BehaviorNode::Status Selector::Execute(float deltaTime, Blackboard* blackboard) {
    for (auto& child : m_children) {
        BehaviorNode::Status status = child->Execute(deltaTime, blackboard);
        if (status != BehaviorNode::Status::FAILURE) {
            return status; // Return SUCCESS or RUNNING
        }
    }
    return BehaviorNode::Status::FAILURE;
}

void Selector::AddBehaviorNode(std::unique_ptr<BehaviorNode> child) {
    m_children.push_back(std::move(child));
}

// Sequence implementation
BehaviorNode::Status Sequence::Execute(float deltaTime, Blackboard* blackboard) {
    for (auto& child : m_children) {
        BehaviorNode::Status status = child->Execute(deltaTime, blackboard);
        if (status != BehaviorNode::Status::SUCCESS) {
            return status; // Return FAILURE or RUNNING
        }
    }
    return BehaviorNode::Status::SUCCESS;
}

void Sequence::AddBehaviorNode(std::unique_ptr<BehaviorNode> child) {
    m_children.push_back(std::move(child));
}

// ConditionNode implementation
ConditionNode::ConditionNode(ConditionFunction condition) 
    : m_condition(condition) {}

BehaviorNode::Status ConditionNode::Execute(float deltaTime, Blackboard* blackboard) {
    return m_condition() ? BehaviorNode::Status::SUCCESS : BehaviorNode::Status::FAILURE;
}

// ActionNode implementation
ActionNode::ActionNode(ActionFunction action) 
    : m_action(action) {}

BehaviorNode::Status ActionNode::Execute(float deltaTime, Blackboard* blackboard) {
    return m_action(deltaTime);
}

// BehaviorTree implementation
BehaviorTree::BehaviorTree(std::unique_ptr<BehaviorNode> rootNode)
    : m_root(std::move(rootNode)) {}

void BehaviorTree::Update(float deltaTime, Blackboard* blackboard) {
    if (m_root) {
        m_root->Execute(deltaTime, blackboard);
    }
}