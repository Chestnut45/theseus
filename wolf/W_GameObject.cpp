#include "W_GameObject.h"

namespace wolf
{

GameObject::GameObject(Scene& scene, GameObjectID id)
    : m_scene(scene), m_id(id)
{
}

GameObject::~GameObject()
{
}

void GameObject::AddChild(GameObject& object)
{
    // Get the object's parent
    GameObject* parent = object.GetParent();
    if (parent)
    {
        // Early out if the object is already one of our children
        if (parent == this) return;

        // Remove existing relationship
        parent->RemoveChild(object);
    }

    // Update references
    m_children.push_back(&object);
    object.m_parent = this;
}

void GameObject::RemoveChild(GameObject& object)
{
    // Find the child object in our list of children
    const auto& it = std::find(m_children.begin(), m_children.end(), &object);
    if (it != m_children.end())
    {
        // If found, remove and update child's parent
        object.m_parent = nullptr;
        m_children.erase(it);
    }
}

void GameObject::DeleteAllChildren()
{
    // Iterate all children backwards
    for (int i = m_children.size() - 1; i >= 0; --i)
    {
        m_children[i]->Delete();
    }
}

};