#pragma once
#include "common/base_math.h"
#include "common/allocator.h"

#define KEYBINDINGS(_)                                                                                                                     \
    _(move_up, SDL_SCANCODE_W)                                                                                                             \
    _(move_left, SDL_SCANCODE_A)                                                                                                           \
    _(move_down, SDL_SCANCODE_S)                                                                                                           \
    _(move_right, SDL_SCANCODE_D)                                                                                                          \
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

enum ActionID {
    KEYBINDINGS(ACTION) 
    count,
};

struct Keybind {
    ActionID action_id;
    SDL_Scancode scancode;
};

#define KEYBIND(a, b) Keybind{ActionID_##a, b},
constexpr Array<Keybind, ActionID::count> default_keybindings = {KEYBINDINGS(KEYBIND)};

#define AS_STRING(a, b) #a,
constexpr Array<const char*, ActionID::count> action_names = {KEYBINDINGS(AS_STRING)};

struct Input {
    float2 mouse_pos{};
    float wheel{};

    Slice<u8> input_text;

    Array<bool, SDL_SCANCODE_COUNT> keyboard_repeat{};
    Array<SDL_Scancode, ActionID::count> keybindings;
    Array<bool, SDL_SCANCODE_COUNT> keyboard_up{};
    Array<bool, SDL_SCANCODE_COUNT> keyboard_down{};

    SDL_MouseButtonFlags button_down_flags{};
    SDL_MouseButtonFlags button_up_flags{};

    const bool* keyboard_held = SDL_GetKeyboardState(NULL);
    SDL_MouseButtonFlags button_held_flags{};

    SDL_Keymod mod_state;
};

void input_begin_frame(Input* input);
void input_end_frame(Input* input);
void input_set_ctx(Input* input);
void input_init_keybinds(Input* input, Array<Keybind, ActionID::count> keybinds);
float2 input_mouse_position();

bool input_action_down(int action_id);
bool input_action_held(int action_id);
bool input_action_up(int action_id);

bool input_key_down(const SDL_Scancode& scan_code);
bool input_key_held(const SDL_Scancode& scan_code);
bool input_key_up(const SDL_Scancode& scan_code);
bool input_key_down_repeat(const SDL_Scancode& scan_code);

bool input_modifier(const SDL_Keymod modifiers);

bool input_mouse_down(SDL_MouseButtonFlags button);
bool input_mouse_held(SDL_MouseButtonFlags button);
bool input_mouse_up(SDL_MouseButtonFlags button);
