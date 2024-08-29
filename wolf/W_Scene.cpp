#include "W_Scene.h"

namespace wolf
{

// Scene implementation

Scene::Scene()
{
}

Scene::~Scene()
{
}

Scene::Object& Scene::CreateObject()
{
    ObjectID id = m_registry.create();
    return m_registry.emplace<Object>(id, *this, id);
}

Scene::Object* Scene::GetObject(ObjectID id)
{
    return m_registry.try_get<Object>(id);
}

void Scene::Delete(ObjectID id)
{
    // Destroy the object and all of its components
    Object* p_object = GetObject(id);
    if (p_object)
    {
        // Delete all child objects first
        // NOTE: Done in reverse order since the vector
        // is modified immediately on deletion
        for (int i = p_object->m_children.size() - 1; i >= 0; i--)
        {
            Delete(p_object->m_children[i]->GetID());
        }

        // Remove any dangling references from the hierarchy
        Object* parent = p_object->GetParent();
        if (parent)
        {
            parent->RemoveChild(*p_object);
        }

        // Destroy the internal entity and all components
        m_registry.destroy(id);
    }
}

void Scene::Clear()
{
    m_registry.clear();
}

// Scene::Object implementation

Scene::Object::Object(Scene& scene, Scene::ObjectID id)
    : m_scene(scene), m_id(id)
{
}

Scene::Object::~Object()
{
}

void Scene::Object::AddChild(Scene::Object& object)
{
    // Get the object's parent
    Object* parent = object.GetParent();
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

void Scene::Object::RemoveChild(Scene::Object& object)
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

}