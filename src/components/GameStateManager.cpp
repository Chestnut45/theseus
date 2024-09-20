#include "GameStateManager.h"
#include "GameState.h"
#include <stack>
 
void GameStateManager::PushState(GameState* state)
{
    if (!m_stateStack.empty()) {
        m_stateStack.top()->Pause();
    }
    m_stateStack.push(state);
    state->Enter();
}

void GameStateManager::PopState()
{
    if (!m_stateStack.empty()) {
        m_stateStack.top()->Exit();
        delete m_stateStack.top();
        m_stateStack.pop();
        if (!m_stateStack.empty()) {
            m_stateStack.top()->Resume();
        }
    }
}

void GameStateManager::ClearAndPushState(GameState* state)
{
    Clear();
    PushState(state);
}

GameState* GameStateManager::GetActiveState() const
{
    return m_stateStack.empty() ? nullptr : m_stateStack.top();
}

void GameStateManager::Update(float delta)
{
    if (!m_stateStack.empty()) {
        m_stateStack.top()->Update(delta);
    }
}

void GameStateManager::Render()
{
    if (!m_stateStack.empty()) {
        m_stateStack.top()->Render();
    }
}

void GameStateManager::Clear()
{
    while (!m_stateStack.empty()) {
        m_stateStack.top()->Exit();
        delete m_stateStack.top();
        m_stateStack.pop();
    }
}
