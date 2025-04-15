#pragma once

#define default_camera \
((Camera2D) {\
    {0, 0},\
    float2_scale((float2){1 * 4.0f/3.0f, 1}, 12.0f),\
})

#define ChunkWidth 8
#define ChunkSize ChunkWidth * ChunkWidth

typedef struct Tile {
    bool is_set;
    TextureID texture;
    i32 x;
    i32 y;
    u32 w;
    u32 h;
} Tile;

typedef struct Chunk {
    float2 position;
    Tile tiles[ChunkSize];
} Chunk;

typedef struct DeleteEntityEvent {
    EntityIndex handle;
} DeleteEntityEvent;
slice_def(DeleteEntityEvent);

typedef struct CreateEntityEvent {
    Entity entity;
} CreateEntityEvent;
slice_def(CreateEntityEvent);

#define MaxTickEvents 64

typedef struct Tick Tick;
struct Tick {
    Entity entities[MaxEntities];
    PlayerInput inputs[MaxPlayers];
    u32 input_count;
    ClientID client_ids[MaxPlayers];
    CreateEntityEvent create_events[MaxTickEvents];
    u32 create_event_count;
    DeleteEntityEvent delete_events;
    u32 delete_event_count;
    u32 tick;
    u64 total_size; // header + slice data
};
ring_def(Tick);

// typedef struct TickQueue {
//     u8* buffer;
//     u64 capacity;
//     u64 size;
//     u64 count;
//     u64 start;
//     u64 end;
//
//     bool end_should_be_less_than_start;
//
//     Tick* head;
//     Tick* tail;
// } TickQueue;

typedef struct Scene Scene;
struct Scene {
    System* sys;
    Input tick_input; // accumulates input events over multiple frames for cases where render freq > tick freq its here for a reason pls dont delete

    UI ui;

    Camera2D camera;

    Server local_server;
    GameState predicted_state;


    Ring_Tick history;

    ClientID client_id;


    bool finished_init_sync;

    // Slice_EntityIndex pending_delete_list;
    // Slice_Entity pending_create_list;
    SnapshotMessage latest_snapshot;

    u32 last_total_sent_data;
    u32 last_total_received_data;
    u32 up_bandwidth; // bytes/second
    u32 down_bandwidth;

    double acc_2;

    bool online_mode;
    bool debug_draw;

    bool edit_mode;

    double accumulator;
    u64 frame;
    double time;
    u32 current_tick;

    // bool ents_modified_since_last_tick;


    b2DebugDraw m_debug_draw;

    bool inventory_open;
    bool moving_spell;
    u16 spell_move_src;
    u16 spell_move_dst;
    bool move_submit;

    bool paused;
    EntityHandle player_handle;

    Arena* level_arena;
    Arena tick_arena;

    ENetHost* client;
    ENetPeer* server;
};

void scene_init(Scene* scene, Arena* level_arena, System* sys, bool online, String8 ip_address);
