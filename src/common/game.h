#pragma once
#include "glm/ext/vector_float2.hpp"
#include "glm/glm.hpp"
#include <EASTL/fixed_vector.h>

enum class PlayerAnimationState {
    Idle,
    Moving,
    Count,
};

struct PlayerInput {
    bool up;
    bool down;
    bool left;
    bool right;
    bool fire;
    glm::vec2 cursor_world_pos;
};

constexpr int max_player_count = 8;
struct Player {
    int health = 3;
    glm::vec2 position;
    PlayerAnimationState anim_state;
    // eastl::fixed_vector<int, max_player_count> free_list;
};

// struct Bullet {
//     glm::vec2 position;
//     glm::vec2 direction_normal;
// }

constexpr int bullets_capacity = 16;

struct Bullet {
    glm::vec2 position;
    glm::vec2 direction_normalized;
    float create_time;
};


struct State {
    bool players_active[max_player_count];
    Player players[max_player_count];

    bool bullets_active[bullets_capacity];
    Bullet bullets[bullets_capacity];
};

void update_state(State& state, PlayerInput inputs[max_player_count], double time, double dt);

void create_player(State& state);

void remove_player(State& state, int player_index);
