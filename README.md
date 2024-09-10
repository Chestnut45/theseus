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

The `wolf::App` abstract base class module has a few new concrete methods and flags for window management that can be used by derived classes:

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

The `wolf::Input` module now handles all input for the program, instead of it being handled by wolf::App. The input module has static functions available anywhere in the program for keyboard and mouse input detection. Check out 'wolf/W_Input.h' for the full API.

```C++
// Detect if the spacebar was just pressed this frame
if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE)) { /*...*/ }

// Detect if the left mouse button was just released this frame
if (wolf::Input::IsLMBReleased()) { /*...*/ }

// Get the amount of vertical mouse scroll this frame
float vScroll = wolf::Input::GetMouseScroll().y;
```

## Audio

The `wolf::Audio` module allows very simple access to loading and playing audio files quickly. It's primarily designed for a "fire and forget" style of usage and can be used from anywhere in the program.

```C++
// Play a sound effect at half volume in the left channel
wolf::Audio::Play("data/sounds/sfx.mp3" /* file path */,
                   false /* no loop */,
                   0.5f /* half volume */,
                   -1.0f /* left channel */
                   );

// Play a looping song at default volume and pan
wolf::Audio::Play("data/sounds/song.wav", true);

// Stop all currently-playing instances of a sound file
wolf::Audio::Stop("data/sounds/song2.ogg");

// Preload a large audio file from disk to be played later without incurring a load on first play
wolf::Audio::Load("data/sounds/largeFile.FLAC");
```

## RNG

The `wolf::RNG` module is an instantiable, seedable pseudo random number generator.

```C++
// Create an instance of the rng with the seed 12345
wolf::RNG rng(12345);

// Get a uniformly distributed random boolean
bool vBool = rng.FlipCoin();

// Get a uniformly distributed random float between 0 and 1 (inclusive)
float vFloat = rng.NextFloat(0.0f, 1.0f);

// Get a uniformly distributed random int between 32 and 64 (inclusive)
int vInt = rng.NextInt(32, 64);
```

## Timer

The `wolf::Timer` module is a high-resolution timer that can be used to delay or schedule repeated events or actions.

```C++
// Create and start a timer
wolf::Timer timer;
timer.Start();

// Later in update loop...

// Do something when 1 second has elapsed
if (timer.Elapsed() >= 1.0f)
{
    // Multiple ways to stop / continue using the timer...

    // Elapsed will remain some value >= 1.0f and the timer will stop
    timer.Stop();

    // Elapsed will reset to 0 and the timer will NOT continue running
    timer.Reset();

    // Elapsed will reset to 0 and the timer WILL continue running
    timer.Restart();
}
```

## Shapes

The `W_Shapes.h` header has a few lightweight classes for representing 2D shapes and detecting collisions between them.

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

The `wolf::EventManager` module can be used anywhere in the program to send events of any type to registered listeners. Instead of making a general purpose "Event" class with expensive string hashing for event parameter creation and retrieval, the event queues and listeners are templated so you can create new event types trivially.

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
    void MyEventHandler(const MyEvent& event)
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
wolf::EventManager::AddListener<MyEvent, MyListener, &MyListener::MyEventHandler>(instance);

// Removing listeners follows the exact same syntax:
wolf::EventManager::RemoveListener<MyEvent, MyListener, &MyListener::MyEventHandler>(instance);
```

If you'd like every instance of a certain type to automatically listen for specific events, you could add a listener in the constructor, and remove it in the destructor. This comes with the caveat of needing to write move/copy constructors and assignment operators for your type (or deleting them if the type never has to be copied or moved, which should be the case for most game components).

To actually send out events to all registered listeners, you can either trigger an event immediately, or enqueue events to dispatch later. The TriggerEvent and EnqueueEvent functions are also templated by event type, but it can be deduced by the argument you pass so there's no need to explicitly state the template type.

```C++
// Dispatch an event immediately
wolf::EventManager::TriggerEvent(MyEvent(123, &someGameObject));

// Queue events for later
wolf::EventManager::EnqueueEvent(MyEvent(456, &anotherObject));
wolf::EventManager::EnqueueEvent(MyEvent(789, &anotherObject));

// Dispatch all queued events of a certain type
wolf::EventManager::Dispatch<MyEvent>();

// Dispatch all queued events, regardless of type
wolf::EventManager::Dispatch();
```

## Scene

The `wolf::Scene` module can be used as a container to create and manage a hierarchy of game objects with components of any type. This is an example of an object-focused entity component system, and will be the backbone for the game's systems. We added a few core components to wolf with built-in behaviour such as Transform2D, Sprite2D, and Camera2D.

```C++
// Create an empty scene
wolf::Scene scene;

// Create an empty game object
wolf::GameObject& object1 = scene.CreateObject();

// Create some objects with a Transform2D component
auto& object2 = scene.CreateObject2D();
auto& object3 = scene.CreateObject2D();

// Add a sprite component to the object
auto& sprite = pObject->AddComponent<wolf::Sprite2D>("data/textures/sprite.png");

// Attach a camera to a game object and set it as the active camera in the scene
// TODO: The camera should follow the game object it is attached to, if any
auto& camera = pObject->AddComponent<wolf::Camera2D>(1280, 720);
scene.SetActiveCamera(camera);

// Delete objects either by reference or by ID
object1.Delete();
scene.DeleteObject(object2.GetID());

// Query for a game object in the scene by ID
wolf::GameObject* pObject = scene.GetObject(object3.GetID());

// Later in the update / render loops...

// Update the scene and all components in it
scene.Update(delta);

// Render all renderable components in the scene using the active camera
scene.Render();
```

In addition to the built-in components, you can trivially create custom component types and attach them to game objects. To use custom components, you'll have to define a component type. Any class or struct is a valid component type as long as it uses at least one public constructor (Compiler-generated default constructor is acceptable too).

```C++
struct CustomComponent
{
    int m_value;
};

// Create and add a component to a game object
// Pass your component's constructor arguments directly to the AddComponent template function
CustomComponent& component = object.AddComponent<CustomComponent>(45);

// Query a game object for a component type
CustomComponent* pComponent = object.GetComponent<CustomComponent>();
if (pComponent)
{
    // Object has a CustomComponent...
}

// Deletion will call the component's destructor automatically
object.DeleteComponent<CustomComponent>();
```

If you want your component to have direct access to the game object that it's attached to (for instance to traverse the game object hierarchy or access other components), there are 2 main options.

By far the simplest option is just to include "W_GameObject.h" and make your component type inherit from wolf::BaseComponent. Doing this will give your component access to a pointer to its game object via the GetGameObject() method. The only restriction is that GetGameObject() will return nullptr inside any constructors for your component, so if your component requires access to the game object immediately during construction, you'll have to use some sort of Init() method after the component is created.

```C++
class AnotherComponent : public wolf::BaseComponent
{
public:
    void SomeFunction()
    {
        GameObject* pParentObject = GetGameObject()->GetParent();
    }

private:
    int m_value;
};
```

The other option is to use dependency injection and just pass a reference to the game object as one of the constructor arguments. This keeps your component inheritence-free, but requires a little extra work when creating components. An upside of this method is that you can pass by reference instead of pointer (since the game object is guaranteed to outlive the component, and components cannot be copied or moved, the reference is never invalidated).

```C++
struct DIComponent
{
    DIComponent(GameObject& object) : m_object(object) {}
    GameObject& m_object;
};

// Creating the component
auto& component = object.AddComponent<DIComponent>(object);
```

To query a game object about whether it has multiple components:

```C++
// True if object has BOTH a Hurtbox and Health component
bool hurtable = object.HasAll<Hurtbox, Health>();

// True if object has EITHER a Sprite2D or Mesh component
bool renderable = object.HasAny<Sprite2D, Mesh>();
```

Game objects can also form a hierarchy by parenting other game objects. This allows for components like Transform2D which walk up the hierarchy to compute the global transformation for child objects.

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

To create a custom system that updates game objects or components, you can use the Scene::Each<T...> method along with structured bindings for very efficient iteration. The first variable bound to the structured binding will be the ID of the game object containing the components, and the subsequent variables will get references to the components themselves, in the same order that you pass the component types as template arguments.

```C++
// Iterate all Sprite2D components
for (auto&&[id, sprite] : scene.Each<Sprite2D>())
{
    // sprite.Render(...) or something
}

// Iterate all objects with at least both a Hitbox2D and Transform2D
for (auto&&[id, hitbox, transform] : scene.Each<Hitbox2D, Transform2D>())
{
    // ...
}

// Iterate all game objects in the scene, regardless of components
// Notice you can iterate the objects themselves directly with the exact same syntax as iterating components
for (auto&&[id, object] : scene.Each<GameObject>())
{
    // Collect only game objects with no children (root objects)
    if (!object.HasChildren())
    {
        // ...
    }
}
```