#pragma once

#include "common/game.h"
#include "common/allocator.h"
#include "renderer.h"
#include "color.h"
#include "font.h"
#include "input.h"
#include "assets.h"
#include "ui.h"

const Camera2D default_camera{
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

    bool edit_mode;


    double accumulator = 0.0;
    u64 frame = {};
    double time = {};
    int current_tick = 0;

    EntityHandle player_handle;


    PlayerInput inputs[max_player_count];
    b2DebugDraw m_debug_draw{};

    bool inventory_open;
    bool moving_spell;
    u16 spell_move_src;
    u16 spell_move_dst;
    bool move_submit;


    Arena* level_arena;
};

// void local_scene_init(LocalScene* scene, Arena* level_arena, Input::Input* input, Renderer* renderer, Font* font);
// void local_scene_update(LocalScene* scene, double delta_time);
// void local_scene_render(LocalScene* scene, Renderer* renderer, SDL_Window* window);

struct State {
    SDL_Window* window;
    Renderer renderer{};
    Input::Input input;
    LocalScene local_scene;

    Array<Font, FontID::font_count> fonts;

    std::chrono::time_point<std::chrono::steady_clock> last_frame_time;

    Arena temp_arena;
    Arena level_arena;

    ENetHost* client;
    ENetPeer* server;

    bool initialized;
};
