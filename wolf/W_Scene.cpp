#include "W_Scene.h"

#include <string>

#include "W_GameObject.h"
#include "W_Sprite2D.h"
#include "W_Transform2D.h"

namespace wolf
{

// Scene implementation

Scene::Scene()
{
}

Scene::~Scene()
{
}

GameObject& Scene::CreateObject()
{
    GameObjectID id = m_registry.create();
    return m_registry.emplace<GameObject>(id, *this, id);
}

GameObject& Scene::CreateObject2D()
{
    GameObjectID id = m_registry.create();
    GameObject& object = m_registry.emplace<GameObject>(id, *this, id);
    object.AddComponent<Transform2D>();
    return object;
}

GameObject* Scene::GetObject(GameObjectID id)
{
    return m_registry.try_get<GameObject>(id);
}

void Scene::DeleteObject(GameObjectID id)
{
    // Destroy the game object and all of its components
    GameObject* p_object = GetObject(id);
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
        GameObject* parent = p_object->GetParent();
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

void Scene::SetActiveCamera(Camera2D& camera)
{
    m_pActiveCamera = &camera;
}

void Scene::RemoveActiveCamera()
{
    m_pActiveCamera = nullptr;
}

void Scene::Update(float delta)
{
    // TODO: Sync all Camera2D components to their transforms (smooth following?)
}

void Scene::Render()
{
    if (!m_pActiveCamera) return;

    // Bind the active camera
    m_pActiveCamera->Bind();

    // Render all sprites with transform components
    for (auto&&[_, sprite, transform] : Each<Sprite2D, Transform2D>())
    {
        sprite.Draw(transform.GetGlobalPosition(), transform.GetGlobalRotation(), transform.GetGlobalScale());
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
    GameObject& object1 = scene.CreateObject();
    GameObject& object2 = scene.CreateObject();
    GameObject& object3 = scene.CreateObject();
    GameObject& object4 = scene.CreateObject();
    GameObject& object5 = scene.CreateObject();
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
    for (auto&&[objectID, object] : scene.Each<GameObject>())
    {
        assert(objectID == object1.GetID());
    }
}

}