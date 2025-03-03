#pragma once

#include "common/game.h"
#include "common/allocator.h"
#include "renderer.h"
#include "color.h"
#include "font.h"
#include "input.h"
#include "assets.h"
#include "ui.h"
#include "scene.h"



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
