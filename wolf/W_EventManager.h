#pragma once

//-----------------------------------------------------------------------------
// File:			W_EventManager.h
// Original Author:	D'Anyil Landry
//
// A static class providing general event dispatching and listener connection.
//-----------------------------------------------------------------------------

#include <entt/signal/dispatcher.hpp>

namespace wolf
{

class EventManager
{

// Public interface
public:

    // Listener management

    // A listener function is a member method of a type that
    // takes an 'EventType &' argument; return value does not matter.
    typedef void (*ListenerFunction)();

    // Connects a listener function to the given event type.
    // Example usage is in the README.md
    template <typename EventType, ListenerFunction function, typename... Listener>
    static void Connect(Listener&&... listener)
    {
        // s_dispatcher.sink<EventType>().connect<ListenerFunction>(listener);
    }

    // Disconnects a listener from the given event type.
    // Example usage is in the README.md
    template <typename EventType, void(*ListenerFunction)(EventType&), typename... Listener>
    static void Disconnect(Listener&&... listener)
    {
        // s_dispatcher.sink<EventType>().disconnect<ListenerFunction>(listener);
    }

    // Disconnects all listeners from the given instance
    template <typename EventType, typename... Listener>
    static void Disconnect(Listener&&... listener)
    {
        // s_dispatcher.sink<EventType>().disconnect(listener...);
    }

    // Event dispatch

    // Dispatches an event of the given type immediately to all connected listeners.
    template <typename EventType>
    static void TriggerEvent(EventType&& event)
    {
        // s_dispatcher.trigger<EventType>(event);
    }

// Implementation
private:

    static inline entt::dispatcher s_dispatcher;
};

// Tests the features and expected behaviour of the event system
void _EventManagerTests();

}