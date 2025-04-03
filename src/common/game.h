#pragma once

typedef enum ReplicationType {
    ReplicationType_Snapshot,
    ReplicationType_Predicted,
} ReplicationType;

typedef enum PlayerAnimState {
    PlayerAnimState_Idle,
    PlayerAnimState_Moving,
    PlayerAnimState_Count,
} PlayerAnimState;

typedef struct PlayerInput PlayerInput;
struct PlayerInput {
    bool up;
    bool down;
    bool left;
    bool right;
    bool fire;
    bool dash;

    bool select_spell[8];
    float2 cursor_world_pos;

    u16 move_spell_src;
    u16 move_spell_dst;
};
slice_def(PlayerInput);
ring_def(PlayerInput);

#define MAX_PLAYER_COUNT 8

typedef enum PlayerState {
    PlayerState_Neutral,
    PlayerState_Dashing,
} PlayerState;

typedef enum SpellType {
    SpellType_NULL,
    SpellType_Fireball,
    SpellType_SpreadBolt,
    SpellType_IceWall,
    SpellType_SniperRifle,
} SpellType;

typedef enum EntityType {
    EntityType_NULL,
    EntityType_Player,
    EntityType_Bullet,
    EntityType_Box,
    EntityType_Dummy,
} EntityType;


typedef u64 EntityFlags;
#define ENTITY_FLAGS(_)\
_(physics)\
_(hittable)\
_(expires)\
_(attack)\
_(player)\
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
slice_def(EntityIndex);

typedef struct EntityHandle EntityHandle;
struct EntityHandle {
    EntityIndex index;
    u32 generation;
};

typedef u64 ClientID;
slice_def(ClientID);

typedef struct Inputs Inputs;
struct Inputs {
    Slice_ClientID ids;
    Slice_PlayerInput inputs;
};

const i32 hotbar_length = 8;
const i32 inventory_rows = 4;

// visual copy on client idk wtf to call it
typedef struct Ghost Ghost;
struct Ghost {
    ReplicationType replication_type;
    TextureID sprite;
    Rect sprite_src;
    bool flip_sprite;

    float2 position;
    u16 health;
    bool show_health;
    bool hit_flash;
};
slice_def(Ghost);

// store definition to recreate
typedef struct ColliderDef {
    f32 half_width;
    f32 half_height;
} ColliderDef;

typedef struct PhysicsComponent {
    ColliderDef collider;
    b2BodyType body_type;
    float2 position;
    float2 linear_velocity;
    f32 linear_damping;
    bool is_sensor;

} PhysicsComponent;

// some fields are for configure initialization
// some are runtime data
// some are references to out of struct data
typedef struct Entity Entity;
struct Entity {
    //automatically set by create_ent()
    bool active;
    u32 generation;
    u32 index;

    EntityType type;
    EntityFlags flags;
    ReplicationType replication_type;

    TextureID sprite;
    Rect sprite_src;
    bool flip_sprite;

    PhysicsComponent physics;
    // dont set
    b2BodyId body_id;

    //player
    ClientID client_id;
    SpellType hotbar[10];
    PlayerState player_state;
    float2 dash_direction;
    u32 dash_end_tick;
    u16 selected_spell;

    // hittable
    i32 health;
    u32 hit_flash_end_tick;

    // expires
    u32 expire_tick;

    EntityHandle owner;
};
slice_def(Entity);
slice_p_def(Entity);

typedef struct Bullet Bullet;
struct Bullet {
    b2BodyId body_id;
    u32 create_tick;
};

const global int box_health = 50;

typedef struct Box Box;
struct Box {
    b2BodyId body_id;
    b2ShapeId shape_id;
    u32 last_hit_tick;
    int health;
};

#define MAX_ENTITIES 256
#define MAX_PREDICTED 128

// typedef struct EntityList {
//     Entity entities[MAX_ENTITIES];
// } EntityList;
//
// ring_def(Slice_Entity);
// typedef struct GameHistory {
//
//     Ring_Slice_Entity history;
// } GameHistory;

typedef struct GameState GameState;
struct GameState {
    Slice_Entity entities;

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

Entity* entity_list_get(Slice_Entity entity_list, EntityHandle handle);
EntityHandle create_entity(GameState* s, Entity entity);
bool entity_is_valid(Slice_Entity entity_list, EntityHandle handle);
// void entities_to_snapshot(Slice_Ghost* ghosts, Slice_Entity ents, u64 current_tick, Slice_pEntity* players);
