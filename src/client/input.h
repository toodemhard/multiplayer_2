#pragma once

namespace Input {

#define KEYBINDINGS(_)                                                                                                                     \
    _(move_up, SDL_SCANCODE_W)                                                                                                             \
    _(move_left, SDL_SCANCODE_A)                                                                                                           \
    _(move_down, SDL_SCANCODE_S)                                                                                                           \
    _(move_right, SDL_SCANCODE_D)                                                                                                          \
    _(dash, SDL_SCANCODE_SPACE)

#define ACTION(a, b) a,

enum ActionID {
    KEYBINDINGS(ACTION) count,
};

struct Keybind {
    uint32_t action_id;
    SDL_Scancode scancode;
};

#define KEYBIND(a, b) Keybind{ActionID::a, b},
constexpr std::array<Keybind, ActionID::count> default_keybindings = {KEYBINDINGS(KEYBIND)};

#define AS_STRING(a, b) #a,
constexpr std::array<const char*, ActionID::count> action_names = {KEYBINDINGS(AS_STRING)};

class Input {
  public:
    void begin_frame();
    void end_frame();

    void init_keybinds(std::array<Keybind, ActionID::count> keybinds);
    bool action_down(int action_id);
    bool action_held(int action_id);
    bool action_up(int action_id);

    bool key_down(const SDL_Scancode& scan_code) const;
    bool key_held(const SDL_Scancode& scan_code) const;
    bool key_up(const SDL_Scancode& scan_code) const;
    bool key_down_repeat(const SDL_Scancode& scan_code) const;

    bool modifier(const SDL_Keymod modifiers) const;

    bool mouse_down(int button) const;
    bool mouse_held(int button) const;
    bool mouse_up(int button) const;

    glm::vec2 mouse_pos{};
    float wheel{};
    std::optional<std::string> input_text;
    std::array<bool, SDL_SCANCODE_COUNT> keyboard_repeat{};

    std::array<SDL_Scancode, ActionID::count> keybindings;

    std::array<bool, SDL_SCANCODE_COUNT> keyboard_up{};
    std::array<bool, SDL_SCANCODE_COUNT> keyboard_down{};

    SDL_MouseButtonFlags button_down_flags{};
    SDL_MouseButtonFlags button_up_flags{};

  private:
    const bool* keyboard_held = SDL_GetKeyboardState(NULL);
    SDL_MouseButtonFlags button_held_flags{};

    SDL_Keymod mod_state;
};

} // namespace Input
