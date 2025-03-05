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
#include "event.h"



// void local_scene_init(LocalScene* scene, Arena* level_arena, Input::Input* input, Renderer* renderer, Font* font);
// void local_scene_update(LocalScene* scene, double delta_time);
// void local_scene_render(LocalScene* scene, Renderer* renderer, SDL_Window* window);

struct Menu {
    Input* input;
    Renderer* renderer;

    Slice<u8> text;

    UI ui;
};

struct State {
    SDL_Window* window;
    Renderer renderer{};
    Input input;
    LocalScene local_scene;
    Menu menu;

    Array<Font, FontID::font_count> fonts;

    std::chrono::time_point<std::chrono::steady_clock> last_frame_time;

    Arena temp_arena;
    Arena level_arena;
    Arena scratch_arenas[scratch_count];

    ENetHost* client;
    ENetPeer* server;

    bool initialized;

    RingBuffer<Event, 10> events;
};

Menu menu_create(Arena* scene_arena, Input* input, SDL_Window* window, Renderer* renderer, const Slice<Font> fonts);
