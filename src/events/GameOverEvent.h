#pragma once

enum class GameOverType
{
    MAIN_MENU,
    EXIT
};

struct GameOverEvent
{
    GameOverType type;
    
    GameOverEvent(GameOverType type) : type(type) {}
};
