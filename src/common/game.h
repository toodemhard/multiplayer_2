#pragma once
#include "allocator.h"
#include "common/base_math.h"
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

    Array<bool, 8> select_spell;
    float2 cursor_world_pos;

    u16 move_spell_src;
    u16 move_spell_dst;
};

constexpr int max_player_count = 8;

enum class PlayerState {
    Neutral,
    Dashing,
};

enum SpellType {
    SpellType_NULL,
    SpellType_Fireball,
    SpellType_SpreadBolt,
    SpellType_IceWall,
    SpellType_SniperRifle,
};

enum class EntityType {
    Player,
    Bullet,
    Box,
    Dummy,
};


typedef u64 EntityFlags;
#define ENTITY_FLAGS(_)\
_(hittable)\
_(expires)\
_(attack)\
_(damage_owner)\

#define ENTITY_FLAG_INDEX(a) EntityFlagIndex_##a,
enum {
    ENTITY_FLAGS(ENTITY_FLAG_INDEX)
};
#define ENTITY_FLAGS_ENUM(a) EntityFlags_##a = 1 << EntityFlagIndex_##a,
enum {
    ENTITY_FLAGS(ENTITY_FLAGS_ENUM)
};

typedef u32 EntityIndex;

struct EntityHandle {
    EntityIndex index;
    u32 generation;
};

typedef u64 ClientID;

struct Inputs {
    Slice<ClientID> ids;
    Slice<PlayerInput> inputs;
};

constexpr i32 hotbar_length = 8;
constexpr i32 inventory_rows = 4;

// visual copy on client idk wtf to call it
struct Ghost {
    EntityType type;
    float2 position;
    u16 health;
    bool show_health;
    bool hit_flash;
    bool flip_sprite;
};

struct Entity {
    //automatically set by create_ent()
    float2 position;
    bool is_active;
    u32 generation;

    EntityType type;
    EntityFlags flags;

    b2BodyId body_id;

    Array<SpellType, 10> hotbar;
    ClientID client_id;
    PlayerState player_state;
    float2 dash_direction;
    u32 dash_end_tick;
    u16 selected_spell;

    bool flip_sprite;

    // hittable
    i32 health;
    u32 hit_flash_end_tick;

    // expires
    u32 expire_tick;

    EntityHandle owner;
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

// constexpr EntityFlags etbf(EntityComponent component) {
//     return 1 << component;
// }


void state_init(GameState* state, Arena* arena);

void state_update(GameState* state, Arena* temp_arena, Inputs inputs, u32 current_tick, i32 tick_rate);

void create_box(GameState* state, float2 position);

EntityHandle create_player(GameState* state, ClientID client_id);

Entity* entity_list_get(const Slice<Entity>* entity_list, EntityHandle handle);

bool entity_is_valid(const Slice<Entity>* entity_list, EntityHandle handle);

Slice<Ghost> entities_to_snapshot(Arena* tick_arena, const Slice<Entity> ents, u64 current_tick, Slice<Entity*>* players);
