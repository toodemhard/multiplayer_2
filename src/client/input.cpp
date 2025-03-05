#include "pch.h"

#include "input.h"

namespace Input {

void Input::begin_frame() {
    ZoneScoped;

    button_held_flags = SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
    mod_state = SDL_GetModState();

    input_text = {};
}

void Input::end_frame() {
    ZoneScoped;

    keyboard_down = {};
    keyboard_up = {};
    keyboard_repeat = {};

    button_down_flags = {};
    button_up_flags = {};
}

void Input::init_keybinds(std::array<Keybind, ActionID::count> keybinds) {
    for (auto kb : keybinds) {
        keybindings[kb.action_id] = kb.scancode;
    }
}

bool Input::action_down(int action_id) {
    return this->key_down(keybindings[action_id]);
}

bool Input::action_held(int action_id) {
    return this->key_held(keybindings[action_id]);
}

bool Input::action_up(int action_id) {
    return this->key_up(keybindings[action_id]);
}

bool Input::modifier(const SDL_Keymod modifiers) const {
    return (modifiers & mod_state);
}

bool Input::mouse_down(SDL_MouseButtonFlags button) const {
    return (button_down_flags & SDL_BUTTON_MASK(button));
}

bool Input::mouse_held(SDL_MouseButtonFlags button) const {
    return (button_held_flags & SDL_BUTTON_MASK(button)) || mouse_down(button);
}

bool Input::mouse_up(SDL_MouseButtonFlags button) const {
    return (button_up_flags & SDL_BUTTON_MASK(button));
}

bool Input::key_down(const SDL_Scancode& scan_code) const {
    return keyboard_down[scan_code];
}

bool Input::key_held(const SDL_Scancode& scan_code) const {
    return keyboard_held[scan_code];
}

bool Input::key_up(const SDL_Scancode& scan_code) const {
    return keyboard_up[scan_code];
}

bool Input::key_down_repeat(const SDL_Scancode& scan_code) const {
    return keyboard_repeat[scan_code];
}

}
