#pragma once

// void local_scene_init(LocalScene* scene, Arena* level_arena, Input::Input* input, Renderer* renderer, Font* font);
// void local_scene_update(LocalScene* scene, double delta_time);
// void local_scene_render(LocalScene* scene, Renderer* renderer, SDL_Window* window);

typedef struct Menu {
    System* sys;
    Slice_u8 text;
    UI ui;
    bool disable_prediction;
} Menu;

typedef struct State State;
struct State {
    System sys;
    Scene local_scene;
    Menu menu;

    f64 last_frame_time;

    Arena temp_arena;
    Arena persistent_arena;
    Arena level_arena;
    Arena menu_arena;
    Arena scratch_arenas[scratch_count];

    ENetHost* client;
    ENetPeer* server;

    bool initialized;

    SceneType active_scene;
};


Menu menu_create(Arena* scene_arena, System* sys);
