internal Input* input_ctx;

void input_begin_frame(Input* input) {
    input->button_held_flags = SDL_GetMouseState(&input->mouse_pos.x, &input->mouse_pos.y);
    input->mod_state = SDL_GetModState();
}

void input_end_frame(Input* input) {
    zero_struct(&input->input_text);
    zero_struct(&input->keyboard_down);
    zero_struct(&input->keyboard_up);
    zero_struct(&input->keyboard_repeat);
    zero_struct(&input->button_down_flags);
    zero_struct(&input->button_up_flags);
    // input->keyboard_down = {0};
    // input->keyboard_up = {};
    // input->keyboard_repeat = {};
    //
    // input->button_down_flags = {};
    // input->button_up_flags = {};
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

void input_init(Input* input, Keybind* keybinds) {
    for (i32 i = 0; i < ActionID_Count; i ++) {
        Keybind* kb = &keybinds[i];
        input->keybindings[kb->action_id] = kb->scancode;
    }
    input->keyboard_held = SDL_GetKeyboardState(NULL);
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

bool input_key_down(SDL_Scancode scan_code) {
    return input_ctx->keyboard_down[scan_code];
}

bool input_key_held(SDL_Scancode scan_code) {
    return input_ctx->keyboard_held[scan_code];
}

bool input_key_up(SDL_Scancode scan_code) {
    return input_ctx->keyboard_up[scan_code];
}

bool input_key_down_repeat(SDL_Scancode scan_code) {
    return input_ctx->keyboard_repeat[scan_code];
}
