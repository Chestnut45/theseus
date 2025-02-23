#pragma once
#include <glm/glm.hpp>
#include <functional>

namespace std {
    template<>
    struct hash<glm::ivec2> {
        size_t operator()(const glm::ivec2& v) const noexcept {
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
        }
    };
}
