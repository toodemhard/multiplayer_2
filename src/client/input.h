#pragma once

#define KEYBINDINGS(_)\
    _(move_up, SDL_SCANCODE_W)\
    _(move_left, SDL_SCANCODE_A)\
    _(move_down, SDL_SCANCODE_S)\
    _(move_right, SDL_SCANCODE_D)\
    _(dash, SDL_SCANCODE_SPACE)\
    _(hotbar_slot_1, SDL_SCANCODE_1)\
    _(hotbar_slot_2, SDL_SCANCODE_2)\
    _(hotbar_slot_3, SDL_SCANCODE_3)\
    _(hotbar_slot_4, SDL_SCANCODE_4)\
    _(hotbar_slot_5, SDL_SCANCODE_5)\
    _(hotbar_slot_6, SDL_SCANCODE_6)\
    _(hotbar_slot_7, SDL_SCANCODE_7)\
    _(hotbar_slot_8, SDL_SCANCODE_8)\

#define ACTION(a, b) ActionID_##a,

typedef enum ActionID {
    KEYBINDINGS(ACTION) 
    ActionID_Count,
} ActionID;

typedef struct Keybind Keybind;
struct Keybind {
    ActionID action_id;
    SDL_Scancode scancode;
};

typedef enum A {
    A_a,
    A_b,
} A;

A aaa[2] = {
    (A)A_a,
    A_b,
};

#define KEYBIND(a, b) {ActionID_##a, b},
Keybind default_keybindings[ActionID_Count] = {KEYBINDINGS(KEYBIND)};

#define AS_STRING(a, b) #a,
const char* action_names[ActionID_Count] = {KEYBINDINGS(AS_STRING)};

typedef struct Input Input;
struct Input {
    float2 mouse_pos;
    float wheel;

    String input_text;
    SDL_Scancode keybindings[ActionID_Count];

    bool keyboard_repeat[SDL_SCANCODE_COUNT];
    bool keyboard_up[SDL_SCANCODE_COUNT];
    bool keyboard_down[SDL_SCANCODE_COUNT];

    SDL_MouseButtonFlags button_down_flags;
    SDL_MouseButtonFlags button_up_flags;

    const bool* keyboard_held;
    SDL_MouseButtonFlags button_held_flags;

    SDL_Keymod mod_state;
};

void input_begin_frame(Input* input);
void input_end_frame(Input* input);
void input_set_ctx(Input* input);
Input* input_get_ctx();
void input_init(Input* input, Keybind* keybinds);
float2 input_mouse_position();

bool input_action_down(int action_id);
bool input_action_held(int action_id);
bool input_action_up(int action_id);

bool input_key_down(SDL_Scancode scan_code);
bool input_key_held(SDL_Scancode scan_code);
bool input_key_up(SDL_Scancode scan_code);
bool input_key_down_repeat(SDL_Scancode scan_code);

bool input_modifier(const SDL_Keymod modifiers);

bool input_mouse_down(SDL_MouseButtonFlags button);
bool input_mouse_held(SDL_MouseButtonFlags button);
bool input_mouse_up(SDL_MouseButtonFlags button);
