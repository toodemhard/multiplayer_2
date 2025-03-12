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

    System* sys = &state->sys;


    arena_init(&state->temp_arena, arena_alloc(&god_allocator, megabytes(5)), megabytes(5));
    arena_init(&state->level_arena, arena_alloc(&god_allocator, megabytes(5)), megabytes(5));
    state->menu_arena = arena_suballoc(&god_allocator, megabytes(2));

    for (i32 i = 0; i < scratch_count; i++)  {
        state->scratch_arenas[i] = arena_suballoc(&god_allocator, megabytes(1));
    }

    auto flags = SDL_WINDOW_INPUT_FOCUS; // | SDL_WINDOW_ALWAYS_ON_TOP; // | SDL_WINDOW_FULLSCREEN;

    {
        ZoneNamedN(a, "Create Window", true);
            sys->window = SDL_CreateWindow("ye", window_width, window_height, flags);
        if (sys->window == NULL) {
            SDL_Log("CreateWindow failed: %s", SDL_GetError());
        }
    }

    if (init_renderer(&sys->renderer, sys->window) != 0) {
        panic("failed to initialize renderer");
    }

    renderer_set_ctx(&sys->renderer);
    

    // Texture textures[ImageID_Count];
    std::future<void> texture_futures[ImageID_Count];

    auto image_to_texture = [](Renderer* renderer, ImageID image_id) {
        int width, height, comp;
        unsigned char* image = stbi_load(image_paths[(int)image_id], &width, &height, &comp, STBI_rgb_alpha);
        load_texture(image_id_to_texture_id(image_id), Image{.w = width, .h = height, .data = image});
    };

    for (i32 i = ImageID_NULL + 1; i < ImageID_Count; i++) {
        texture_futures[i] = std::async(std::launch::async, image_to_texture, &sys->renderer, (ImageID)i);
        // image_to_texture(state->renderer, (ImageID)i);
    }

    std::future<void> font_futures[FontID::font_count];

    for (i32 i = 0; i < FontID::font_count; i++) {
        font_futures[i] = std::async(std::launch::async, font_load, &sys->fonts[i], std::ref(sys->renderer), FontID::Avenir_LT_Std_95_Black_ttf, 512, 512, 64);
        // font::load_font(&state->fonts[i], state->renderer, FontID::Avenir_LT_Std_95_Black_ttf, 512, 512, 64);
    }

    input_init_keybinds(&sys->input, default_keybindings);

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

    sys->fonts_view = slice_create_view(&sys->fonts.data[0], sys->fonts.length);

    // double last_render_duration = 0.0;

    state->menu = menu_create(&state->menu_arena, sys);

    state->initialized = true;
    return true;
}

Menu menu_create(Arena* scene_arena, System* sys) {
    Menu menu = {
        .sys=sys,
        // .text = cstr_to_string("127.0.0.1"),
        .text = slice_create<u8>(scene_arena, 512),
    };
    ui_init(&menu.ui, scene_arena, sys->fonts_view, &sys->renderer);

    SDL_StartTextInput(sys->window);

    slice_push_range(&menu.text, cstr_to_string("127.0.0.1"));

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

void text_field_input(Slice<u8>* text) {
    string_cat(text, input_get_ctx()->input_text);

    if (input_modifier(SDL_KMOD_CTRL) && input_key_down_repeat(SDL_SCANCODE_V)) {
        string_cat(text, cstr_to_string(SDL_GetClipboardText()));
    }

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
    System* sys = menu->sys;

    ui_set_ctx(&menu->ui);

    ui_begin();

    ArenaTemp scratch = scratch_get(0, 0);
    defer(scratch_release(scratch));


    ui_row({
        .font_size = font_px(32),
        .stack_axis = Axis2_Y,
        .position = pos_anchor2(0.5, 0.5),
        // .background_color = {0.1,0.1,0.3,1}
    }) {

        if (ui_button(ui_row_ret({
            .text = "Local",
        }))) {
            ring_buffer_push_back(&sys->events, Event{
                .type = EventType_StartScene,
                .start_scene = {
                    .online = false,
                },
            });
        }





        ui_row({}) {
            ui_row({.text = "Server IP: "});
            UI_Key ip_field;
            float4 border_color = {0.4,0.4,0.4,1};
            ui_row({
                .out_key = &ip_field,
                .text = string_to_cstr(scratch.arena, menu->text),
                .size = {size_ru(16)},
                .border = sides_px(2),
                .border_color = border_color,
            });
            if (ui_is_active(ip_field)) {
                ui_prev_element()->border_color = {0.9,0.9,0.9,1};
                text_field_input(&menu->text);
            }


            // if (ui_is_active(ui_key(ip_field_key))) {
            // }

            if (ui_button(ui_row_ret({
                .text = "Connect",
                .background_color = {1,0,0,1},
            }))) {
                printf("%s\n", string_to_cstr(scratch.arena, menu->text));
                ring_buffer_push_back(&sys->events, Event{
                    .type = EventType_StartScene,
                    .start_scene = {
                        .online = true,
                        .connect_ip = menu->text,
                    },
                });
            }

        }
    }

    ui_end(temp_arena);

    ui_draw(&menu->ui, temp_arena);
}



extern "C" UPDATE(update) {
    auto state = (State*)memory;

    bool quit = false;

    System* sys = &state->sys;

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

    input_begin_frame(&sys->input);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            quit = true;
            break;
        }

        // ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
        case SDL_EVENT_MOUSE_WHEEL:
            sys->input.wheel += event.wheel.y;
            break;
        case SDL_EVENT_KEY_DOWN:
            sys->input.keyboard_repeat[event.key.scancode] = true;

            if (!event.key.repeat) {
                sys->input.keyboard_down[event.key.scancode] = true;
            }
            break;

        case SDL_EVENT_KEY_UP:
            sys->input.keyboard_up[event.key.scancode] = true;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            sys->input.button_down_flags |= SDL_BUTTON_MASK(event.button.button);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.down) {
                break;
            }
            sys->input.button_up_flags |= SDL_BUTTON_MASK(event.button.button);
            break;
        case SDL_EVENT_TEXT_INPUT: {
            sys->input.input_text = cstr_to_string(event.text.text);
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

    while (sys->events.length > 0) {
        Event union_event = ring_buffer_pop_front(&sys->events);
        
        switch (union_event.type) {
        case EventType_StartScene: {
            arena_reset(&state->level_arena);
            Slice<Font> fonts_view = slice_create_view(&sys->fonts.data[0], sys->fonts.length);

            auto event = union_event.start_scene; // for once auto is useful, anon struct
            
            scene_init(&state->local_scene, &state->level_arena, sys, event.online, event.connect_ip);
            state->active_scene = SceneType_Game;
        } break;
        case EventType_QuitScene: {
            state->active_scene = SceneType_Menu;
            arena_reset(&state->menu_arena);
            state->menu = menu_create(&state->menu_arena, sys);
        } break;

        }

    }

    input_set_ctx(&sys->input);
    renderer_set_ctx(&sys->renderer);

    // update
    {
        // multi_scene.update(delta_time);
        // local_scene_update(&state->local_scene, &state->temp_arena, delta_time);
        // state->local_scene.update(delta_time);
    }

    // render
    {
        auto render_begin_time = std::chrono::high_resolution_clock::now();
        begin_rendering(sys->window, &state->temp_arena);


        if (state->active_scene == SceneType_Menu) {
            menu_update(&state->menu, &state->temp_arena);
        }

        if (state->active_scene == SceneType_Game) {
            scene_update(&state->local_scene, &state->temp_arena, delta_time);
        }
        // //
        // // auto string = std::format("render: {:.3f}ms", last_render_duration * 1000.0);
        // // font::draw_text(renderer, font, string.data(), 24, {20, 30});
        // //

        end_rendering();

        // last_render_duration = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() -
        // render_begin_time).count();
    }

    input_set_ctx(&sys->input);
    bool reload = false;
    if (input_key_down(SDL_SCANCODE_R) && input_modifier(SDL_KMOD_CTRL)) {
        reload = true;
    }

    input_end_frame(&sys->input);

    is_reloaded = false;

    return {
        .quit = quit,
        .reload = reload,
    };
}
