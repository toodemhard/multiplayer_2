internal Input* input_ctx;

void input_begin_frame(Input* input) {
    ZoneScoped;

    input->button_held_flags = SDL_GetMouseState(&input->mouse_pos.x, &input->mouse_pos.y);
    input->mod_state = SDL_GetModState();

    input->input_text = {};
}

void input_end_frame(Input* input) {
    ZoneScoped;

    input->keyboard_down = {};
    input->keyboard_up = {};
    input->keyboard_repeat = {};

    input->button_down_flags = {};
    input->button_up_flags = {};
}

float2 input_mouse_position() {
    return input_ctx->mouse_pos;
}

void input_set_ctx(Input* input) {
    input_ctx = input;
}
Input* input_get_ctx() {
    return input_ctx;
}

void input_init_keybinds(Input* input, Array<Keybind, ActionID::count> keybinds) {
    for (i32 i = 0; i < keybinds.length; i ++) {
        Keybind* kb = &keybinds[i];
        input->keybindings[kb->action_id] = kb->scancode;
    }
    // input->keybindings = keybinds;
}

bool input_action_down(int action_id) {
    return input_key_down(input_ctx->keybindings[action_id]);
}

bool input_action_held(int action_id) {
    return input_key_held(input_ctx->keybindings[action_id]);
}

bool input_action_up(int action_id) {
    return input_key_up(input_ctx->keybindings[action_id]);
}

bool input_modifier(const SDL_Keymod modifiers) {
    return modifiers & input_ctx->mod_state;
}

bool input_mouse_down(SDL_MouseButtonFlags button) {
    return (input_ctx->button_down_flags & SDL_BUTTON_MASK(button));
}

bool input_mouse_held(SDL_MouseButtonFlags button) {
    return (input_ctx->button_held_flags & SDL_BUTTON_MASK(button)) || input_mouse_down(button);
}

bool input_mouse_up(SDL_MouseButtonFlags button) {
    return (input_ctx->button_up_flags & SDL_BUTTON_MASK(button));
}

bool input_key_down(const SDL_Scancode& scan_code) {
    return input_ctx->keyboard_down[scan_code];
}

bool input_key_held(const SDL_Scancode& scan_code) {
    return input_ctx->keyboard_held[scan_code];
}

bool input_key_up(const SDL_Scancode& scan_code) {
    return input_ctx->keyboard_up[scan_code];
}

bool input_key_down_repeat(const SDL_Scancode& scan_code) {
    return input_ctx->keyboard_repeat[scan_code];
}
