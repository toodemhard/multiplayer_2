#pragma once

#include "renderer.h"
#include "color.h"
#include "font.h"
#include "game.h"
#include "client.h"
#include "input.h"
#include "assets.h"
#include "ui.h"
#include <chrono>

const Camera2D default_camera{
    {0, 0},
    glm::vec2{1 * 4.0f/3.0f, 1} * 8.0f,
};

class LocalScene {
public:
    Input::Input* m_input;
    Font* m_font;

    Input::Input m_tick_input;
    Camera2D m_camera = default_camera;

    State m_state{};


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
};
