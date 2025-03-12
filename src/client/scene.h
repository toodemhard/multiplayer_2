#pragma once

#include "event.h"
#include "ui.h"
#include "common/game.h"

const inline Camera2D default_camera = {
    {0, 0},
    float2{1 * 4.0f/3.0f, 1} * 12.0f,
};

constexpr int chunk_width = 8;
constexpr int chunk_size = chunk_width * chunk_width;


struct Tile {
    bool is_set;
    TextureID texture;
    i32 x;
    i32 y;
    u32 w;
    u32 h;
};

struct Chunk {
    float2 position;
    Tile tiles[chunk_width * chunk_width];
};

struct Scene {
    System* sys;
    Input tick_input; // accumulates input events over multiple frames for cases where render freq > tick freq its here for a reason pls dont delete

    UI ui;

    Camera2D camera = default_camera;

    GameState state{};

    u8 last_input_buffer_size;
    Slice<Ghost> latest_snapshot;

    u32 last_total_sent_data;
    u32 last_total_received_data;
    u32 up_bandwidth; // bytes/second
    u32 down_bandwidth;

    double acc_2;

    bool online_mode;

    bool edit_mode;

    double accumulator = 0.0;
    u64 frame = {};
    double time = {};
    int current_tick = 0;

    EntityHandle player_handle;

    b2DebugDraw m_debug_draw{};

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

void scene_init(Scene* scene, Arena* level_arena, System* sys, bool online, String_8 ip_address);
void scene_update(Scene* s, Arena* frame_arena, double delta_time);
