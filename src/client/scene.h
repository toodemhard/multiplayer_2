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

typedef struct Snapshot {
    Ghost ghosts[MaxPredicted];
    u32 tick;
} Snapshot;

ring_def(Snapshot);

typedef struct Scene Scene;
struct Scene {
    System* sys;
    Input tick_input; // accumulates input events over multiple frames for cases where render freq > tick freq its here for a reason pls dont delete

    UI ui;

    Camera2D camera;

    Server local_server;
    GameState offline_state; // fake server for local play
    GameState predicted_state;


    // Ring_PredictionTick prediction_history;

    Ring_Snapshot latency_buff;

    // u8 last_input_buffer_size;

    // Snapshot snapshot;
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
    int current_tick;

    EntityHandle player_handle;

    b2DebugDraw m_debug_draw;

    bool inventory_open;
    bool moving_spell;
    u16 spell_move_src;
    u16 spell_move_dst;
    bool move_submit;

    bool paused;
    Entity player;

    Arena* level_arena;
    Arena tick_arena;

    ENetHost* client;
    ENetPeer* server;
};

void scene_init(Scene* scene, Arena* level_arena, System* sys, bool online, String8 ip_address);
void scene_update(Scene* s, Arena* frame_arena, double delta_time);
