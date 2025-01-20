#pragma once

#include <glm/glm.hpp>
#include <functional>

namespace std {
    template <>
    struct hash<glm::ivec2> {
        size_t operator()(const glm::ivec2& vec) const {
            size_t hashX = std::hash<int>()(vec.x);
            size_t hashY = std::hash<int>()(vec.y);
            return hashX ^ (hashY << 1);
        }
    };
}