#pragma once

typedef enum SceneType {
    SceneType_Menu,
    SceneType_Game,
} SceneType;

typedef enum EventType {
    EventType_NULL,
    EventType_StartScene,
    EventType_QuitScene,
} EventType;

typedef struct GameStartEvent {
    bool online;
    String8 connect_ip;
} GameStartEvent;

typedef struct Event {
    EventType type;
    union {
        GameStartEvent game_start;
    };
} Event;
ring_def(Event);

// more things should use global ctx system

typedef struct System {
    SDL_Window* window;
    Renderer renderer;
    Input input;
    Font fonts[FontID_Count];
    Slice_Font fonts_view;


    Ring_Event events;
} System;
