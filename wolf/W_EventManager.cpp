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

    struct TestListener
    {
        // Example receiver method
        void OnTestEvent(const TestEvent& event)
        {
            Log(event.m_data);
        }
    };

    TestListener listener;

    EventManager::AddListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener);

    EventManager::TriggerEvent(TestEvent(45));

    EventManager::RemoveListener<TestEvent, TestListener, &TestListener::OnTestEvent>(listener);

    EventManager::TriggerEvent(TestEvent(69));
}

}