#include "W_EventManager.h"

#include "W_Logging.h"

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
            Log(event.m_data);
        }

        void OnTestEvent2(const TestEvent2& event)
        {
            Log(event.x, " ", event.p);
        }
    };

    // Add 2 listeners
    TestListener listener;
    TestListener listener2;
    EventManager::AddListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener);
    EventManager::AddListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener2);

    EventManager::TriggerEvent(TestEvent(45));

    EventManager::RemoveListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener);

    EventManager::TriggerEvent(TestEvent(69));

    EventManager::EnqueueEvent(TestEvent(12345));
    EventManager::EnqueueEvent(TestEvent(67890));
    EventManager::EnqueueEvent(TestEvent(42069));

    EventManager::Dispatch<TestEvent>();

    EventManager::EnqueueEvent(TestEvent(12345));
    EventManager::EnqueueEvent(TestEvent(67890));
    EventManager::EnqueueEvent(TestEvent(42069));

    EventManager::AddListener<TestEvent2, TestListener, &TestListener::OnTestEvent2>(listener);
    EventManager::TriggerEvent(TestEvent2(123, nullptr));
    EventManager::RemoveListener<TestEvent2, TestListener, &TestListener::OnTestEvent2>(listener);

    EventManager::Dispatch();

    EventManager::RemoveListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener2);
}

}