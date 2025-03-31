#pragma once

#define default_camera \
((Camera2D) {\
    {0, 0},\
    float2_scale((float2){1 * 4.0f/3.0f, 1}, 12.0f),\
})

#define chunk_width 8
#define chunk_size chunk_width * chunk_width

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
    Tile tiles[chunk_size];
} Chunk;

typedef struct Scene Scene;
struct Scene {
    System* sys;
    Input tick_input; // accumulates input events over multiple frames for cases where render freq > tick freq its here for a reason pls dont delete

    UI ui;

    Camera2D camera;

    GameState state;

    u8 last_input_buffer_size;
    Slice_Ghost latest_snapshot;

    u32 last_total_sent_data;
    u32 last_total_received_data;
    u32 up_bandwidth; // bytes/second
    u32 down_bandwidth;

    double acc_2;

    bool online_mode;

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
