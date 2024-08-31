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

    // Connects a listener to the given event type.
    // A listener is any member method that has an EventType& parameter.
    // Example usage is in the README.md
    template <typename EventType, typename ListenerType, void (ListenerType::*Function)(const EventType&)>
    static void AddListener(ListenerType& listener)
    {
        s_dispatcher.template sink<EventType>().template connect<Function>(listener);
    }

    // Disconnects a listener from the given event type.
    template <typename EventType, typename ListenerType, void (ListenerType::*Function)(const EventType&)>
    static void RemoveListener(ListenerType& listener)
    {
        s_dispatcher.template sink<EventType>().template disconnect<Function>(listener);
    }

    // Event dispatch

    // Dispatches an event of the given type immediately to all connected listeners.
    // NOTE: Execution order of listeners is not guaranteed!
    template <typename EventType>
    static void TriggerEvent(const EventType& event)
    {
        s_dispatcher.trigger(event);
    }

    // Adds an event of the given type to the internal queue.
    template <typename EventType>
    static void EnqueueEvent(const EventType& event)
    {
        s_dispatcher.enqueue(event);
    }

    // Dispatches all queued events of the given type immediately.
    // NOTE: Execution order of listeners is not guaranteed!
    template <typename EventType>
    static void Dispatch()
    {
        s_dispatcher.update<EventType>();
    }

    // Dispatches all queued events of any type immediately.
    // NOTE: Execution order of listeners is not guaranteed!
    static void Dispatch()
    {
        s_dispatcher.update();
    }

// Implementation
private:

    static inline entt::dispatcher s_dispatcher;
};

// Tests the features and expected behaviour of the event system
void _EventManagerTests();

}