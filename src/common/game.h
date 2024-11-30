#pragma once
#include "glm/ext/vector_float2.hpp"
#include "glm/glm.hpp"
#include <EASTL/fixed_vector.h>

struct PlayerInput {
    bool up;
    bool down;
    bool left;
    bool right;
    bool fire;
};

constexpr int bullets_capacity = 16;
struct Bullets {
    bool alive[bullets_capacity];
    glm::vec2 position[bullets_capacity];
    float create_time[bullets_capacity];
};

struct State {
    eastl::fixed_vector<glm::vec2, 16> players;
    Bullets bullets;
};

void update(State& state, eastl::fixed_vector<PlayerInput, 16>& inputs, double time, double dt);
