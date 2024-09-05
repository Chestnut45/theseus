#include "W_Scene.h"

#include <string>

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

void Scene::DeleteObject(ObjectID id)
{
    // Destroy the object and all of its components
    Object* p_object = GetObject(id);
    if (p_object)
    {
        // Delete all child objects first
        // NOTE: Done in reverse order since the vector
        // is modified immediately on deletion
        for (int i = (int)p_object->m_children.size() - 1; i >= 0; i--)
        {
            DeleteObject(p_object->m_children[i]->GetID());
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

void _SceneTests()
{
    // Component type
    class TestComponent
    {
    public:
        TestComponent(const std::string& string) : m_s(string) {}
        const std::string& GetString() const { return m_s; }
    private:
        std::string m_s;
    };

    // Create the scene and objects
    Scene scene;
    Scene::Object& object1 = scene.CreateObject();
    Scene::Object& object2 = scene.CreateObject();
    Scene::Object& object3 = scene.CreateObject();
    Scene::Object& object4 = scene.CreateObject();
    Scene::Object& object5 = scene.CreateObject();
    object1.AddChild(object2);
    object2.AddChild(object3);
    object1.AddComponent<int>(45);
    object1.AddComponent<float>(3.1415926535f);
    object2.AddComponent<std::string>("test string");
    object3.AddComponent<TestComponent>("test component");
    object4.AddComponent<TestComponent>("test component the second");
    object5.AddComponent<TestComponent>("test component 3: the squeaquel");

    // Test hierarchy and component system
    assert(object1.GetChildren()[0] == &object2 && "object2 should be a child of object1");
    assert(object2.GetParent() == &object1 && "object1 should be the parent of object2");
    assert(*object1.GetComponent<int>() == 45 && "primitive types should behave as components");
    assert(object1.GetComponent<std::string>() == nullptr && "we never added a string to 1, should return null");
    assert(*object2.GetComponent<std::string>() == "test string" && "string data should be stable");
    assert(object3.GetComponent<TestComponent>()->GetString() == "test component" && "custom component types as well");

    // Test deletion
    object2.Delete();
    object4.Delete();
    object5.Delete();
    assert(object1.GetChildren().size() == 0 && "deleting a child should update the parent");

    // Testing iterating objects with multiple component types

    // Iterate all components of a single type
    for (auto&&[objectID, i] : scene.Each<int>())
    {
        assert(objectID == object1.GetID());
        assert(i = 45);
    }

    // Iterate all objects with AT LEAST all the given component types
    for (auto&&[objectID, i, f] : scene.Each<int, float>())
    {
        assert(objectID == object1.GetID());
        assert(i = 45);
        assert(f = 3.1415926535f);
    }

    // Iterate all objects in the scene
    for (auto&&[objectID, object] : scene.EachObject())
    {
        assert(objectID == object1.GetID());
    }
}

}