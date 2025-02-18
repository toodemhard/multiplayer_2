#pragma once

#include "common/allocator.h"
#include "renderer.h"
#include "color.h"
#include "font.h"
#include "client.h"
#include "input.h"
#include "assets.h"
#include "ui.h"
#include <chrono>

const Camera2D default_camera{
    {0, 0},
    glm::vec2{1 * 4.0f/3.0f, 1} * 8.0f,
};

constexpr int chunk_width = 8;
constexpr int chunk_size = chunk_width * chunk_width;


struct Tile {
    bool is_set;
    TextureID texture;
    u16 x;
    u16 y;
    u16 w;
    u16 h;
};

struct Chunk {
    glm::vec2 position;
    Tile tiles[chunk_width * chunk_width];
};

struct LocalScene {
    Input::Input* m_input;
    Font* m_font;
    Renderer* m_renderer;

    Input::Input m_tick_input;
    Camera2D m_camera = default_camera;

    GameState m_state{};

    bool m_edit_mode;


    double accumulator = 0.0;
    uint32_t frame = {};
    double time = {};
    int current_tick = 0;

    EntityHandle player_handle;


    PlayerInput inputs[max_player_count];
    b2DebugDraw m_debug_draw{};

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

    std::chrono::time_point<std::chrono::steady_clock> last_frame_time;

    std::vector<RGBA> rects;

    Arena temp_arena;
    Arena level_arena;
};
