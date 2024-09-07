# Setup Instructions

## Windows

### Setting up Dev Environment

1) Install Visual Studio Community Edition
2) Install Visual Studio Code
3) Install CMake (At least version 3.12)
4) Ensure CMake is on your PATH
5) Install C/C++ VS Code Extension (0.28.3 or above)
6) Install CMake Tools VS Code extension (version 1.4.1 or above)
7) Install CMake VS Code extension (0.0.17 or above)

### Building

1) Open the project in VS Code
2) Ctrl+Shift+P then type/choose CMake: Configure
3) Choose one of the available options:
    A) If you want to use MSVC: "Visual Studio Community 2022 Release - amd64"
    B) If you want to use GCC/Mingw: "GCC 13.2.0 x86_64-w64-mingw32 (mingw64)"
4) Ctrl+Shift+P then type/choose CMake: Build
5) Hit F5 to run in debugger if all compiled well (if not, make sure your compiler / kit from step 3 supports C++20)
6) You may need to choose your build target (theseus) from the dropdown the first time

## Linux

### Setting up Dev Environment

1) Install gcc or clang
2) Install Visual Studio Code
3) Install CMake (At least version 3.12)
4) Ensure CMake is on your PATH
5) Install C/C++ VS Code Extension (0.28.3 or above)
6) Install CMake Tools VS Code extension (version 1.4.1 or above)
7) Install CMake VS Code extension (0.0.17 or above)

### Building

1) Open the project in VS Code
2) Ctrl+Shift+P then type/choose CMake: Configure
3) Choose one of the available options (e.g. gcc)
4) Ctrl+Shift+P then type/choose CMake: Build
5) Hit F5 to run in debugger if all compiled well
6) You may need to choose your build target (theseus) from the dropdown the first time
7) If you see errors about including "GL/glu.h", you may also need libglu1 development libraries (apt-get install libglu1-mesa libglu1-mesa-dev)

# Wolf Extensions

## App

The 'W_App' module has a few new concrete methods and flags for window management that can be used by derived app classes:

```C++
// Anywhere inside your app's update loop...

// Enables or disables fullscreen.
SetFullscreen(true);

// Enables or disables vertical sync
SetVsync(false);

// Display a debug performance graph and
// window options window using ImGui.
ShowDebug();

// Detect when the window was resized
// NOTE: This will be set to true automatically when the
// window is resized, but must be reset manually.
if (m_windowResized)
{
    // Do stuff...

    // Reset the flag manually
    m_windowResized = false;
}
```

## Input

The 'W_Input' module now handles all input for the program, instead of it being handled by wolf::App. The input module has static functions available anywhere in the program for keyboard and mouse input detection. Check out 'wolf/W_Input.h' for the full API.

```C++
// Detect if the spacebar was just pressed this frame
if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE)) { /*...*/ }

// Detect if the left mouse button was just released this frame
if (wolf::Input::IsLMBReleased()) { /*...*/ }

// Get the amount of vertical mouse scroll this frame
float vScroll = wolf::Input::GetMouseScroll().y;
```

## Audio

The 'W_Audio' module allows very simple access to loading and playing audio files quickly. It's primarily designed for a "fire and forget" style of usage and can be used from anywhere in the program.

```C++
// Play a sound effect at half volume in the left channel
wolf::Audio::Play("data/sfx.mp3" /* file path */, false /* no loop */, 0.5f /* half volume */, -1.0f /* left channel */);

// Play a looping song at default volume and pan
wolf::Audio::Play("data/song.wav" /* file path */, true /* loop */);

// Stop all currently-playing instances of a sound file
wolf::Audio::Stop("data/song2.ogg");

// Preload a large audio file from disk to be played later without incurring a load on first play
wolf::Audio::Load("data/largeFile.FLAC");
```

## RNG

The 'W_RNG' module is an instantiable, seedable pseudo random number generator.

```C++
// Create an instance of the rng with the seed 12345
wolf::RNG rng(12345);

// Get a random boolean
bool vBool = rng.FlipCoin();

// Get a float between 0 and 1 (inclusive)
float vFloat = rng.NextFloat(0.0f, 1.0f);

// Get an int between 32 and 64 (inclusive)
int vInt = rng.NextInt(32, 64);
```

## Shapes

The 'W_Shapes' module has a few lightweight classes for representing 2D shapes and detecting collisions between them.

```C++
// Create some circles at the origin
wolf::Circle circle(glm::vec2(0, 0), 0.5f);
wolf::Circle circle2(glm::vec2(0, 0), 40.0f)

// Test for intersections
bool contains = circle.Intersects(glm::vec2(0, 0));
bool intersects = circle.Intersects(circle2);

// Create a rectangle from (-1, -1) to (8, 8)
wolf::Rectangle rectangle(-1 /* left */, 8 /* top */, 8 /* right */, -1 /* bottom */);

// Create the same rectangle but by defining origin and size
wolf::Rectangle rectangle2(glm::vec2(-1, 8) /* top left coordinate */, glm::vec2(9, 9) /* width and height */);

// Test for intersections
bool testPoint = rectangle.Intersects(glm::vec2(0, 0));
bool testRect = rectangle.Intersects(wolf::Rectangle(0, 0, 8, 8));
```

## EventManager

The 'W_EventManager' module can be used anywhere in the program to send events of any type to registered listeners. Instead of making a general purpose "Event" class with expensive string hashing for event parameter creation and retrieval, the event queues and listeners are templated so you can create new event types trivially.

Any type is a valid event type, but POD structs are the simplest to use.

```C++
// Example event type
struct MyEvent
{
    int m_data;
    wolf::Scene::Object* m_pObject;
};
```

In order to register a listener, you'll need to create a listener function as a member method of some class or struct. A listener member function is any member function that accepts a const reference to some event type.

```C++
// Example listener type
struct MyListener
{
    // Example listener function for 'MyEvent' events
    void TheActualFunction(const MyEvent& event)
    {
        // Do something with the event parameters...
        auto pSprite = event.m_pObject->GetComponent<Sprite>();
        // ...
    }
};

// Create an instance of your type with the listener method.
MyListener instance;

// Add the listener by passing the event type, listener type, and a
// pointer to the listener function as template arguments, and a
// reference to the actual instance of the listener as a regular argument.
wolf::EventManager::AddListener<MyEvent, MyListener, &MyListener::TheActualFunction>(instance);

// Removing listeners follows the exact same syntax:
wolf::EventManager::RemoveListener<MyEvent, MyListener, &MyListener::TheActualFunction>(instance);
```

If you'd like every instance of a certain type to automatically listen for events, you could call AddListener in the constructor, and RemoveListener in the destructor.

To actually send out events to all registered listeners, you can either trigger an event immediately, or enqueue events to dispatch later. The TriggerEvent and EnqueueEvent functions are also templated by event type, but it can be deduced by the argument you pass so there's no need to explicitly state the template type.

```C++
// Dispatch an event immediately
wolf::EventManager::TriggerEvent(MyEvent(123, &someSceneObject));

// Queue events for later
wolf::EventManager::EnqueueEvent(MyEvent(456, &anotherObject));
wolf::EventManager::EnqueueEvent(MyEvent(789, &anotherObject));

// Dispatch all queued events of a certain type
wolf::EventManager::Dispatch<MyEvent>();

// Dispatch all queued events, regardless of type
wolf::EventManager::Dispatch();
```

## Scene

The 'W_Scene' module can be used as a container to manage a hierarchy of game objects with components of any type.

To create and delete objects in a scene:

```C++
// Create an empty scene
wolf::Scene scene;

// Create some game objects in the scene
// NOTE: It's important to capture by reference here, since game objects are not copyable or moveable.
wolf::GameObject& object1 = scene.CreateObject();
auto& object2 = scene.CreateObject();
auto& object3 = scene.CreateObject();

// Delete objects either way
object2.Delete();
scene.DeleteObject(object3.GetID());

// Query for a game object in the scene by ID
wolf::GameObject* pObject = scene.GetObject(object1.GetID());
```

Then, to add some components to those objects, you'll have to define a component type. Any class or struct is a valid component type as long as it uses at least one public constructor.

```C++
class CustomComponent
{
public:
    CustomComponent(int value) : m_value(value) {}
    int GetValue() const { return m_value; }

private:
    int m_value;
};
```

To construct a new component and add it to a game object:

```C++
// Pass your component's constructor arguments directly to the AddComponent template function
// NOTE: Not safe to call if the component already exists!
CustomComponent& component = object.AddComponent<CustomComponent>(45);
```

To query an object for a component:

```C++
CustomComponent* pComponent = object.GetComponent<CustomComponent>();
if (pComponent)
{
    // Object has a CustomComponent...
}
```

To delete a component from a game object:

```C++
// Safe to call even if the component does not exist
object.DeleteComponent<CustomComponent>();
```

To query a game object about whether it has multiple components:

```C++
// True if object has BOTH a Collider and Health component
bool hurtable = object.HasAll<Collider, Health>();

// True if object has EITHER a Sprite or Mesh component
bool renderable = object.HasAny<Sprite, Mesh>();
```

To change hierarchical relationships between game objects:

```C++
// Create some objects
auto& hand = scene.CreateObject();
auto& thumb = scene.CreateObject();
auto& finger = scene.CreateObject();

// Add / remove child objects
hand.AddChild(thumb);
hand.AddChild(finger);
hand.RemoveChild(finger);

// Query relationships
hand.GetParent(); // nullptr
thumb.GetParent(); // &hand
finger.GetParent(); // nullptr

// Get a const reference to a vector of pointers to all child objects
// NOTE: Type is const std::vector<wolf::GameObject*>&
const auto& children = object1.GetChildren();
```

To create a system that updates game objects or components, you can use the Scene::Each<T...> method along with structured bindings for very efficient iteration. The first variable bound will be the game object ID of the game object containing the components, and the subsequent variables will get references to the components themselves, in the same order that you pass the component types as template arguments.

```C++
// Iterate all Sprite2D components
for (auto&&[id, sprite] : scene.Each<Sprite2D>())
{
    // sprite.Render(...) or something
}

// Iterate all objects with at least both component types
for (auto&&[id, hitbox, transform] : scene.Each<Hitbox2D, Transform2D>())
{
    // ...
}

// Iterate all game objects in the scene, regardless of components
for (auto&&[id, object] : scene.Each<GameObject>())
{
    // Collect only game objects with no children
    if (!object.HasChildren())
    {
        // ...
    }
}
```