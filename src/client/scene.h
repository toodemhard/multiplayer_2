#pragma once

#include "input.h"
#include "renderer.h"
#include "font.h"
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

struct LocalScene {
    Input::Input* input;
    Input::Input tick_input; // accumulates input events over multiple frames for cases where render freq > tick freq its here for a reason pls dont delete
    Renderer* renderer;

    Slice<Font> fonts;

    UI ui;

    Camera2D camera = default_camera;

    GameState state{};
    Slice<Ghost> latest_snapshot;

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


    Arena* level_arena;

    ENetHost* client;
    ENetPeer* server;
};

void local_scene_init(LocalScene* scene, Arena* level_arena, Input::Input* input, Renderer* renderer, Slice<Font> fonts);
void local_scene_update(LocalScene* s, Arena* frame_arena, double delta_time);
void local_scene_render(LocalScene* s, Renderer* renderer, Arena* frame_arena, SDL_Window* window);
