#pragma once

#include <W_GameObject.h>

struct LightToggleEvent {
    wolf::GameObjectID m_CallerID;
    bool m_bOn;
};