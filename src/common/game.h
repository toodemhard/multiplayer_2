#pragma once
#include "box2d/id.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/glm.hpp"
#include <EASTL/fixed_vector.h>
#include "box2d/box2d.h"
#include <vector>

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
    bool dash;
    glm::vec2 cursor_world_pos;
};

constexpr int max_player_count = 8;

enum class PlayerState {
    Neutral,
    Dashing,
};

struct Player {
    int health = 3;
    // glm::vec2 position;
    PlayerState state;
    glm::vec2 dash_direction;
    double dash_start_time;

    PlayerAnimationState anim_state;

    b2BodyId body_id;
    // eastl::fixed_vector<int, max_player_count> free_list;
};

// struct Bullet {
//     glm::vec2 position;
//     glm::vec2 direction_normal;
// }
enum class EntityType {
    Bullet,
    Box,
};

struct EntityRef {
    EntityType type;
    int index;
};

constexpr int bullets_capacity = 16;
constexpr int boxes_capacity = 64;


struct Bullet {
    EntityRef sensor_ref;
    b2BodyId body_id;
    float create_time;
};

struct Box {
    EntityRef sensor_ref;
    b2BodyId body_id;
    b2ShapeId shape_id;
};


struct State {
    bool players_active[max_player_count];
    Player players[max_player_count];

    bool bullets_active[bullets_capacity];
    Bullet bullets[bullets_capacity];


    b2WorldId world_id;
    b2BodyId ground_id;

    bool boxes_active[boxes_capacity];
    Box boxes[boxes_capacity];

    // b2BodyId body_id;
};

glm::vec2 b2vec_to_glmvec(b2Vec2 vec);

void init_state(State& state);

void update_state(State& state, PlayerInput inputs[max_player_count], double time, double dt);

using PlayerID = int;
PlayerID create_player(State& state);

void remove_player(State& state, int player_index);
