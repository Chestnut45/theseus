# Setup Instructions

## Windows

### Setting up Dev Environment

1) Install Visual Studio Community Edition 
2) Install Visual Studio Code
3) Install CMake
4) Ensure CMake is on your PATH
5) Install C/C++ VS Code Extension (0.28.3 or above)
6) Install CMake Tools VS Code extension (version 1.4.1 or above)
7) Install CMake VS Code extension (0.0.17 or above)

### Building

1) Open the project in VS Code
2) Ctrl+Shift+P then type/choose CMake: Configure
3) Choose one of the available options (e.g. gcc)
4) Ctrl+Shift+P then type/choose CMake: Build
5) Hit F5 to run in debugger if all compiled well (if not, make sure your compiler / kit from step 3 supports C++20)
6) You may need to choose your build target (theseus) from the dropdown the first time

## Linux

### Setting up Dev Environment

1) Install gcc or clang
2) Install Visual Studio Code
3) Install CMake
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

The wolf::App abstract class has a few new concrete methods and flags you can use:

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
if (m_windowResized)
{
    // Do stuff...

    // Reset the flag
    m_windowResized = false;
}
```

## Scene

The wolf::Scene class can be used as a container to manage a hierarchy of game objects with arbitrary type components.

To create and delete objects in a scene:

```C++
// Create an empty scene
wolf::Scene scene;

// Create some objects in the scene
// NOTE: It's important to capture by reference here, since scene objects are not copyable or moveable.
wolf::Scene::Object& object = scene.CreateObject();
auto& object2 = scene.CreateObject();
auto& object3 = scene.CreateObject();

// Delete an object
object2.Delete();
scene.DeleteObject(object3.GetID());
```

If you want to store object IDs or get access to an object's scene:

```C++
// Get an object's ID to store somewhere
wolf::Scene::ObjectID id = object.GetID();

// Get a reference to the object's scene
wolf::Scene& sceneRef = object.GetScene();

// Query for an object in the scene by ID
wolf::Scene::Object* pObject = scene.GetObject(id);
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

To construct a new component and add it to an object:

```C++
// Pass your component's constructor arguments directly to the AddComponent template function
// NOTE: You can only add a component to an object if it does not already have one of that type!
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

To delete a component from an object:
```C++
object.DeleteComponent<CustomComponent>();
```

To query an object about multiple components:

```C++
// True if object has BOTH a Collider and Health component
bool hurtable = object.HasAll<Collider, Health>();

// True if object has EITHER a Sprite or Mesh component
bool renderable = object.HasAny<Sprite, Mesh>();
```

To change hierarchical relationships between objects:

```C++
// Create some objects
auto& object1 = scene.CreateObject();
auto& object2 = scene.CreateObject();
auto& object3 = scene.CreateObject();

// Set relationships
object1.AddChild(object2);
object1.AddChild(object3);
object1.RemoveChild(object3);

// Query relationships
object1.GetParent(); // nullptr
object2.GetParent(); // &object1
object3.GetParent(); // nullptr

// Get a const reference to a vector of pointers to all child objects
const std::vector<wolf::Scene::Object*>& children = object1.GetChildren();
```

To create a game system that updates objects or components, you can use the Scene::EachObject() and Scene::Each<T> methods along with structured bindings for very efficient iteration. The first variable bound will be the object ID, and the second will be a reference to the actual component:

```C++
// Iterate all Sprite components
for (auto&&[objectID, sprite] : scene.Each<Sprite>())
{
    // sprite.Render(...) or something
}

// Iterate all objects in the scene
for (auto&&[objectID, object] : scene.EachObject())
{
    // Collect only objects with no children
    if (object.HasChildren())
    {
        // ...
    }
}

// TODO: Iterate objects that have at least X, Y, Z, components
```

## EventManager

the wolf::EventManager is in progress...

## Input

## Audio

## ImGui

## RNG