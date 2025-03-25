#pragma once

enum SceneType {
    SceneType_Menu,
    SceneType_Game,
};

enum EventType {
    EventType_NULL,
    EventType_StartScene,
    EventType_QuitScene,
};

struct Event {
    EventType type;
    union {
    struct {
        bool online;
        String8 connect_ip;
    } start_scene;
    };
};

// more things should use global ctx system

struct System {
    SDL_Window* window;
    Renderer renderer;
    Input input;
    Array<Font, FontID::font_count> fonts;
    Slice<Font> fonts_view;
    RingBuffer<Event, 10> events;
};
