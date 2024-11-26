#pragma once
#include "glm/ext/vector_float2.hpp"
#include "glm/glm.hpp"
#include <EASTL/fixed_vector.h>

struct PlayerInput {
    bool up;
    bool down;
    bool left;
    bool right;
};

struct State {
    eastl::fixed_vector<glm::vec2, 16> players;
};
