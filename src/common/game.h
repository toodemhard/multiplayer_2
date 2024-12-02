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
    glm::vec<2, int> mouse_pos;
};

constexpr int max_player_count = 8;
struct Players {
    bool alive[max_player_count];
    glm::vec2 position[max_player_count];

    // eastl::fixed_vector<int, max_player_count> free_list;
};

constexpr int bullets_capacity = 16;
struct Bullets {
    bool alive[bullets_capacity];
    glm::vec2 position[bullets_capacity];
    float create_time[bullets_capacity];
};


struct State {
    Players players;
    Bullets bullets;
};

void update(State& state, PlayerInput inputs[], double time, double dt);

void create_player(State& state);

void remove_player(State& state, int player_index);
