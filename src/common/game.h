#pragma once
#include "common/allocator.h"
#include "types.h"

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

    bool slot0;
    bool slot1;
    bool slot2;
    bool slot3;
    bool slot4;
    glm::vec2 cursor_world_pos;
};

constexpr int max_player_count = 8;

enum class PlayerState {
    Neutral,
    Dashing,
};

enum class EntityType {
    Player,
    Bullet,
    Box,
    Dummy,
};

typedef u64 EntityComponentFlags;

typedef u32 EntityIndex;

struct EntityHandle {
    EntityIndex index;
    u32 generation;
};

enum EntityComponent {
    attack,
    hittable,
    expires,
};

struct Entity {
    //automatically set by create_ent()
    bool is_active;
    u32 generation;

    EntityType entity_type;
    EntityComponentFlags flags;

    b2BodyId body_id;

    u32 player_id;
    PlayerState player_state;
    glm::vec2 dash_direction;
    u32 dash_end_tick;
    u16 current_spell;

    bool flip_sprite;

    // hittable
    i32 health;
    u32 hit_flash_end_tick;

    // expires
    u32 expire_tick;
};

constexpr int bullets_capacity = 16;
constexpr int boxes_capacity = 64;


struct Bullet {
    b2BodyId body_id;
    u32 create_tick;
};

constexpr int box_health = 50;

struct Box {
    b2BodyId body_id;
    b2ShapeId shape_id;
    u32 last_hit_tick;
    int health;
};


struct GameState {
    Slice<Entity> entities;


    b2WorldId world_id;
    b2BodyId ground_id;
};

constexpr EntityComponentFlags etbf(EntityComponent component) {
    return 1 << component;
}

glm::vec2 b2vec_to_glmvec(b2Vec2 vec);

void state_init(GameState* state, Arena* arena);

void state_update(GameState* state, Arena* temp_arena, PlayerInput inputs[max_player_count], u32 current_tick, i32 tick_rate);

void create_box(GameState* state, glm::vec2 position);

EntityHandle create_player(GameState* state);

Entity* entity_list_get(const Slice<Entity>* entity_list, EntityHandle handle);

