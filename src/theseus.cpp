#include "theseus.h"

class TestComponent
{
public:
    TestComponent(const std::string& string) : m_s(string) {}
    const std::string& GetString() const { return m_s; }
private:
    std::string m_s;
};

// Application entrypoint
int main(int, char**)
{
    Theseus app;
    app.Run();
    return 0;
}

Theseus::Theseus() : App("Theseus", 1280, 720)
{
    // TODO: Initialization logic

    // DEBUG: Scene system test
    wolf::Scene::Object& object1 = m_scene.CreateObject();
    wolf::Scene::Object& object2 = m_scene.CreateObject();
    wolf::Scene::Object& object3 = m_scene.CreateObject();
    object1.AddChild(object2);
    object2.AddChild(object3);
    object1.AddComponent<int>(45);
    object2.AddComponent<std::string>("test string");
    object3.AddComponent<TestComponent>("test component");
    assert(object1.GetChildren()[0] == &object2 && "object2 should be a child of object1");
    assert(object2.GetParent() == &object1 && "object1 should be the parent of object2");
    assert(*object1.GetComponent<int>() == 45 && "primitive types should behave as components");
    assert(object1.GetComponent<float>() == nullptr && "we never added a float, should return null");
    assert(*object2.GetComponent<std::string>() == "test string" && "string data should be stable");
    assert(object3.GetComponent<TestComponent>()->GetString() == "test component" && "custom component types as well");
    object2.Delete();
    assert(object1.GetChildren().size() == 0 && "deleting a child should update the parent");
}

Theseus::~Theseus()
{
    // TODO: Shutdown logic
}

void Theseus::Update(float delta)
{
    // Debug hotkeys
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) Shutdown();
    if (wolf::Input::IsKeyDown(GLFW_KEY_GRAVE_ACCENT)) ShowDebug();

    // DEBUG: Audio test
    // if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE)) wolf::Audio::Play("data/omg.mp3");

    
    // TODO: Update logic
}

void Theseus::Render()
{
    // Clear the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: Rendering logic
}