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
    template <typename EventType>
    static void TriggerEvent(EventType&& event)
    {
        s_dispatcher.trigger(event);
    }

// Implementation
private:

    static inline entt::dispatcher s_dispatcher;
};

// Tests the features and expected behaviour of the event system
void _EventManagerTests();

}