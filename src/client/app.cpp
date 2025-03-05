#include "pch.h"

#include "panic.h"
#include "renderer.h"
#include "stb_image.h"
#include "scene.h"

#include "app.h"
#include "exports.h"

#include "common/net_common.h"
#include "common/game.h"

// #if defined(TRACY_ENABLE)
//     #pragma message("MY_MACRO is defined")
// #endif


// const int asdfhkj = 13413432;
// constexpr int tick_rate = 128;

// constexpr double fixed_dt = 1.0 / tick_rate;
constexpr double speed_up_fraction = 0.05;

constexpr int window_width = 1024;
constexpr int window_height = 768;

constexpr float idle_period = 0.2;
constexpr int idle_count = 2;
constexpr int idle_period_ticks = idle_period * tick_rate;
constexpr int idle_cycle_period_ticks = idle_period * idle_count * tick_rate;

const float hit_flash_duration = 0.1;

static bool is_reloaded = true;




bool init(void* memory) {
    ZoneScoped;
    using namespace renderer;
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = b2Vec2{0.0f, 0.0f};
    auto world_id = b2CreateWorld(&world_def);

    auto body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    auto body_id = b2CreateBody(world_id, &body_def);

    auto ground_box = b2MakeBox(2.0, 0.5);
    auto ground_shape_def = b2DefaultShapeDef();
    b2CreatePolygonShape(body_id, &ground_shape_def, &ground_box);

    b2World_Step(world_id, 0.1, 4);


    std::cout << std::filesystem::current_path().string() << '\n';

    Arena god_allocator{};
    arena_init(&god_allocator, memory, megabytes(16));
    State* state = (State*)arena_alloc(&god_allocator, sizeof(State));
    new (state) State{};

    arena_init(&state->temp_arena, arena_alloc(&god_allocator, megabytes(5)), megabytes(5));
    arena_init(&state->level_arena, arena_alloc(&god_allocator, megabytes(5)), megabytes(5));

    for (i32 i = 0; i < scratch_count; i++)  {
        state->scratch_arenas[i] = arena_suballoc(&god_allocator, megabytes(1));
    }

    auto flags = SDL_WINDOW_INPUT_FOCUS; // | SDL_WINDOW_ALWAYS_ON_TOP; // | SDL_WINDOW_FULLSCREEN;

    {
        ZoneNamedN(a, "Create Window", true);
            state->window = SDL_CreateWindow("ye", window_width, window_height, flags);
        if (state->window == NULL) {
            SDL_Log("CreateWindow failed: %s", SDL_GetError());
        }
    }

    if (init_renderer(&state->renderer, state->window) != 0) {
        panic("failed to initialize renderer");
    }
    

    // Texture textures[ImageID_Count];
    std::future<void> texture_futures[ImageID_Count];

    auto image_to_texture = [](Renderer& renderer, ImageID image_id) {
        int width, height, comp;
        unsigned char* image = stbi_load(image_paths[(int)image_id], &width, &height, &comp, STBI_rgb_alpha);
        renderer::load_texture(&renderer, image_id_to_texture_id(image_id), Image{.w = width, .h = height, .data = image});
    };

    for (i32 i = ImageID_NULL + 1; i < ImageID_Count; i++) {
        texture_futures[i] = std::async(std::launch::async, image_to_texture, std::ref(state->renderer), (ImageID)i);
        // image_to_texture(state->renderer, (ImageID)i);
    }

    std::future<void> font_futures[FontID::font_count];

    for (i32 i = 0; i < FontID::font_count; i++) {
        font_futures[i] = std::async(std::launch::async, font::load_font, &state->fonts[i], std::ref(state->renderer), FontID::Avenir_LT_Std_95_Black_ttf, 512, 512, 64);
        // font::load_font(&state->fonts[i], state->renderer, FontID::Avenir_LT_Std_95_Black_ttf, 512, 512, 64);
    }

    input_init_keybinds(&state->input, default_keybindings);

    float2 player_position{0, 0};

    // state->local_scene.frame = 12312;
    // (&state->local_scene)->m_input = &state->input;





    // InitializeYojimbo();

    for (int i = ImageID_NULL + 1; i < ImageID_Count; i++) {
        texture_futures[i].wait();
    }

    for (i32 i = 0; i < FontID::font_count; i++) {
        font_futures[i].wait();
    }

    state->last_frame_time = std::chrono::high_resolution_clock::now();

    Slice<Font> fonts_view = slice_create_view(&state->fonts.data[0], state->fonts.length);

    // double last_render_duration = 0.0;

    state->menu = menu_create(&state->level_arena, &state->input, state->window, &state->renderer, fonts_view);

    state->initialized = true;
    return true;
}

Menu menu_create(Arena* scene_arena, Input* input, SDL_Window* window, Renderer* renderer, const Slice<Font> fonts) {
    Menu menu = {
        .input=input,
        .renderer=renderer,
    };
    ui_init(&menu.ui, scene_arena, fonts, renderer);

    SDL_StartTextInput(window);

    menu.text = slice_create<u8>(scene_arena, 512);
    slice_push(&menu.text, (u8)'d');

    return menu;
}

bool f;

// Slice<Slice<u8>> str_key(const char* str) {
//     return cstr_to_string(str)
//
// }

// #define 
// constexpr source_key() {
//     
// }

void text_field_input(Input* input, Slice<u8>* text) {
    string_cat(text, &input->input_text);

    if (input_modifier(SDL_KMOD_CTRL) && input_key_down_repeat(SDL_SCANCODE_BACKSPACE)) {
        i32 cut_to = -1;
        bool next_whitespace = false;
        for (i32 i = text->length - 1; i >= 0; (i--, cut_to = i)) {
            if (next_whitespace && (*text)[i] == ' ') {
                break;
            } else if ((*text)[i] != ' ') {
                next_whitespace = true;
            }
        }

        text->length = cut_to + 1;

    } else if (input_key_down_repeat(SDL_SCANCODE_BACKSPACE)) {
        if (text->length > 0) {
            text->length--;
        }
    }
}


void menu_update(Menu* menu, Arena* temp_arena) {

    // if (menu->input->key_down(SDL_SCANCODE_S)) {
    //     f = !f;
    // }

    ui_set_ctx(&menu->ui);

    ui_begin();

    ArenaTemp scratch = scratch_get(0, 0);
    defer(scratch_release(scratch));

    UI_Element idk = {
        .size = {size_px(100), size_px(100)},
        .background_color = {1,0,1,1},
    };
    ui_row(idk){}


    ui_row({
        // .text = ,
        // .font_size = font_px(32),
        .size = {size_px(600), size_px(200)},
        .background_color = {0.1,0.1,0.3,1}
    }) {
        if(f) {
    ui_row({
        .size = {size_px(300), size_px(300)},
        .background_color = {0.5,0,0.8},
    }) {}

        }

        ui_row({
            .size = {size_grow(), size_grow()},
            .border = sides_px(2),
            .border_color = {1,0,0,1},
        }) {}
        UI_Key ip_field;
        ui_row({
            .out_key = &ip_field,
            .text = string_to_cstr(scratch.arena, menu->text),
            .size = {size_px(100), size_px(20)},
        });

        if (ui_is_active(ip_field)) {
            text_field_input(menu->input, &menu->text);
        }

        if (ui_button(ui_row_ret({
            .text = "send",
            .background_color = {1,0,0,1},
        }))) {
            printf("%s\n", string_to_cstr(scratch.arena, menu->text));
        }

        ui_row({
            .size = {size_grow(), size_grow()},
            .border = sides_px(2),
            .border_color = {1,0,0,1},
        }) {}
        // ui_push_row({
        //   .border_size = {size_px(2), size_px(2), size_px(2), size_px(2)},
        //   .border_color = {1, 0, 0, 1},
        //   })

    }

    ui_end(temp_arena);

    ui_draw(&menu->ui, menu->renderer, temp_arena);
}



extern "C" UPDATE(update) {
    auto state = (State*)memory;

    bool quit = false;

    if (!state->initialized) {
        if (!init(memory)) {
            quit = true;
        }
    }

    // need to restore global variables on dll reload
    if (is_reloaded) {
        for (i32 i = 0; i < scratch_count; i++)  {
            scratch_arenas[i] = &state->scratch_arenas[i];
        }

    }

    arena_reset(&state->temp_arena);


    auto this_frame_time = std::chrono::high_resolution_clock::now();
    double delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(this_frame_time - state->last_frame_time).count();
    state->last_frame_time = this_frame_time;

    input_begin_frame(&state->input);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            quit = true;
            break;
        }

        // ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
        case SDL_EVENT_MOUSE_WHEEL:
            state->input.wheel += event.wheel.y;
            break;
        case SDL_EVENT_KEY_DOWN:
            state->input.keyboard_repeat[event.key.scancode] = true;

            if (!event.key.repeat) {
                state->input.keyboard_down[event.key.scancode] = true;
            }
            break;

        case SDL_EVENT_KEY_UP:
            state->input.keyboard_up[event.key.scancode] = true;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            state->input.button_down_flags |= SDL_BUTTON_MASK(event.button.button);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.down) {
                break;
            }
            state->input.button_up_flags |= SDL_BUTTON_MASK(event.button.button);
            break;
        case SDL_EVENT_TEXT_INPUT: {
            state->input.input_text = cstr_to_string(event.text.text);
            // u8 y = *event.text.text;
            // i32 x = strlen(event.text.text);
            // printf("%s\n", event.text.text);
        } break;
        case SDL_EVENT_TEXT_EDITING: {
            printf("%s\n", event.edit.text);
        } break;
        default:
            break;
        }
    }

    while (state->events.length > 0) {
        Event event = ring_buffer_pop_front(&state->events);
        
        switch (event.type) {
        case EventType_StartScene: {
            Slice<Font> fonts_view = slice_create_view(&state->fonts.data[0], state->fonts.length);
            local_scene_init(&state->local_scene, &state->level_arena, &state->input, &state->renderer, fonts_view, cstr_to_string("ahlfkjahdsf"));
        } break;

        }

    }

    input_set_ctx(&state->input);

    // update
    {
        // multi_scene.update(delta_time);
        // local_scene_update(&state->local_scene, &state->temp_arena, delta_time);
        // state->local_scene.update(delta_time);
    }

    // render
    {
        auto render_begin_time = std::chrono::high_resolution_clock::now();
        renderer::begin_rendering(&state->renderer, state->window, &state->temp_arena);

        menu_update(&state->menu, &state->temp_arena);
        // if (is_reloaded) {
        //     create_box(&state->local_scene.state, float2{2, 1});
        //     std::cout << "reloaded\n";
        //
        //     // create_player(&state->local_scene.state);
        //
        //     // SDL_SetWindowAlwaysOnTop(state->window, true);
        // }

        // multi_scene.render(renderer, window, textures);
        // local_scene_render(&state->local_scene, &state->renderer, &state->temp_arena, state->window);

        // //
        // // auto string = std::format("render: {:.3f}ms", last_render_duration * 1000.0);
        // // font::draw_text(renderer, font, string.data(), 24, {20, 30});
        // //

        renderer::end_rendering(&state->renderer);

        // last_render_duration = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() -
        // render_begin_time).count();
    }

    input_set_ctx(&state->input);
    bool reload = false;
    if (input_key_down(SDL_SCANCODE_R) && input_modifier(SDL_KMOD_CTRL)) {
        reload = true;
    }

    input_end_frame(&state->input);

    is_reloaded = false;

    return {
        .quit = quit,
        .reload = reload,
    };
}
