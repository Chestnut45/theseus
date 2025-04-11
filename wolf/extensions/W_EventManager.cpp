//-----------------------------------------------------------------------------
// File:			W_EventManager.cpp
// Original Author:	D'Anyil Landry
//
// A static class providing general event dispatching and listener connection.
//-----------------------------------------------------------------------------

#include "W_EventManager.h"

namespace wolf
{

void _EventManagerTests()
{
    struct TestEvent
    {
        int m_data;
    };

    struct TestEvent2
    {
        int x;
        int* p;
    };

    struct TestListener
    {
        void OnTestEvent(const TestEvent& event)
        {
            listenerCalls1++;
        }

        void OnTestEvent2(const TestEvent2& event)
        {
            listenerCalls2++;
        }

        int listenerCalls1 = 0;
        int listenerCalls2 = 0;
    };

    // Add 2 listeners
    TestListener listener;
    TestListener listener2;
    EventManager::AddListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener);
    EventManager::AddListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener2);

    EventManager::TriggerEvent(TestEvent(45));

    EventManager::RemoveListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener);

    EventManager::TriggerEvent(TestEvent(123));

    EventManager::EnqueueEvent(TestEvent(12345));
    EventManager::EnqueueEvent(TestEvent(67890));
    EventManager::EnqueueEvent(TestEvent());

    EventManager::Dispatch<TestEvent>();

    EventManager::EnqueueEvent(TestEvent(12345));
    EventManager::EnqueueEvent(TestEvent(67890));
    EventManager::EnqueueEvent(TestEvent(123));

    EventManager::AddListener<TestEvent2, TestListener, &TestListener::OnTestEvent2>(listener);
    EventManager::TriggerEvent(TestEvent2(123, nullptr));
    EventManager::RemoveListener<TestEvent2, TestListener, &TestListener::OnTestEvent2>(listener);

    EventManager::Dispatch();

    EventManager::RemoveListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener2);

    // Make sure all listener functions fired correctly
    assert(listener.listenerCalls1 == 1);
    assert(listener.listenerCalls2 == 1);
    assert(listener2.listenerCalls1 == 8);
    assert(listener2.listenerCalls2 == 0);
}

}