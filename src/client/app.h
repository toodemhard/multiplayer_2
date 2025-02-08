#pragma once

#include "common/allocator.h"
#include "renderer.h"
#include "color.h"
#include "font.h"
// #include "game.h"
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

class LocalScene {
public:
    Input::Input* m_input;
    Font* m_font;

    Input::Input m_tick_input;
    Camera2D m_camera = default_camera;

    State m_state{};

    bool m_edit_mode;


    double accumulator = 0.0;
    uint32_t frame = {};
    double time = {};
    int current_tick = 0;


    PlayerInput inputs[max_player_count];

    b2DebugDraw m_debug_draw{};

    LocalScene() {};
    LocalScene(Input::Input* input, Renderer* renderer);

    void update(double delta_time);
    void render(Renderer& renderer, SDL_Window* window);
};

struct DLL_State {
    SDL_Window* window;
    Renderer renderer{};
    Input::Input input;
    LocalScene local_scene;

    std::chrono::time_point<std::chrono::steady_clock> last_frame_time;

    std::vector<RGBA> rects;

    Arena temp_arena;
};
