#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


#include "assets/inc.h"


#include "base/base_inc.h"
#include "os/os.h"

#include "base/base_inc.c"
#include "os/os.c"


#include "common/inc.h"
#include "common/inc.c"

#include "color.h"
#include "exports.h"
#include "input.h"
#include "renderer.h"
#include "font.h"
#include "event.h"
#include "ui.h"
#include "scene.h"
#include "app.h"

#include "font.c"
#include "input.c"
#include "renderer.c"
#include "scene.c"
#include "ui.c"

const global double speed_up_fraction = 0.05;

const global int window_width = 1024;
const global int window_height = 768;

#define IDLE_PERIOD 0.2
#define IDLE_COUNT 2;
// const global int idle_period_ticks = IDLE_PERIOD * TICK_RATE;
// const global int idle_cycle_period_ticks = IDLE_PERIOD * IDLE_COUNT * TICK_RATE;

// constexpr float hit_flash_duration = 0.1;

global bool is_reloaded = true;

bool init(void* memory) {
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){0.0f, 0.0f};
    b2WorldId world_id = b2CreateWorld(&world_def);

    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    b2BodyId body_id = b2CreateBody(world_id, &body_def);

    b2Polygon ground_box = b2MakeBox(2.0, 0.5);
    b2ShapeDef ground_shape_def = b2DefaultShapeDef();
    b2CreatePolygonShape(body_id, &ground_shape_def, &ground_box);

    b2World_Step(world_id, 0.1, 4);


    // std::cout << std::filesystem::current_path().string() << '\n';
    // Renderer r = {};
    // r = {};

    Arena god_allocator = {0};
    arena_init(&god_allocator, memory, memory_size);
    State* state = (State*)arena_alloc(&god_allocator, sizeof(State));
    zero_struct(state);

    System* sys = &state->sys;


    arena_init(&state->temp_arena, arena_alloc(&god_allocator, megabytes(5)), megabytes(5));
    arena_init(&state->level_arena, arena_alloc(&god_allocator, megabytes(5)), megabytes(5));
    state->persistent_arena = arena_suballoc(&god_allocator, megabytes(1));
    state->menu_arena = arena_suballoc(&god_allocator, megabytes(2));

    for (i32 i = 0; i < scratch_count; i++)  {
        state->scratch_arenas[i] = arena_suballoc(&god_allocator, megabytes(2));
    }
    for (i32 i = 0; i < scratch_count; i++)  {
        scratch_arenas[i] = &state->scratch_arenas[i];
    }

    sys->events = ring_alloc(Event, &state->persistent_arena, 32);

    SDL_WindowFlags flags = SDL_WINDOW_INPUT_FOCUS; // | SDL_WINDOW_ALWAYS_ON_TOP; // | SDL_WINDOW_FULLSCREEN;

    {
        // ZoneNamedN(a, "Create Window", true);
            sys->window = SDL_CreateWindow("ye", window_width, window_height, flags);
        if (sys->window == NULL) {
            SDL_Log("CreateWindow failed: %s", SDL_GetError());
        }
    }

    if (init_renderer(&sys->renderer, sys->window) != 0) {
        ASSERT(false);
        // panic("failed to initialize renderer");
    }

    if (enet_initialize() < 0) {
        ASSERT(false);
        // printf("enet failed to init\n");
    }


    renderer_set_ctx(&sys->renderer);
    

    // Texture textures[ImageID_Count];
    // std::future<void> texture_futures[ImageID_Count];

    for (i32 i = ImageID_NULL + 1; i < ImageID_Count; i++) {
        // image_to_texture(&sys->renderer, (ImageID)i);
        int width, height, comp;
        unsigned char* image = stbi_load(image_paths[(int)i], &width, &height, &comp, STBI_rgb_alpha);
        load_texture(image_id_to_texture_id(i), (Image){.w = width, .h = height, .data = image});
        // image_to_texture(state->renderer, (ImageID)i);
    }

    for (i32 i = 0; i < FontID_Count; i++) {
        font_load(&state->persistent_arena, &sys->fonts[i], &sys->renderer, FontID_Avenir_LT_Std_95_Black_ttf, 512, 512, 64);
        // font::load_font(&state->fonts[i], state->renderer, FontID::Avenir_LT_Std_95_Black_ttf, 512, 512, 64);
    }

    input_init(&sys->input, default_keybindings);

    float2 player_position = {0, 0};


    state->last_frame_time = os_now_seconds();

    sys->fonts_view = slice_create_view(Font, sys->fonts, array_length(sys->fonts));

    // double last_render_duration = 0.0;

    state->menu = menu_create(&state->menu_arena, sys);

    state->initialized = true;
    return true;
}

Menu menu_create(Arena* scene_arena, System* sys) {
    Menu menu = {
        .sys=sys,
        // .text = cstr_to_string("127.0.0.1"),
        .text = slice_create(u8, scene_arena, 512),
    };
    ui_init(&menu.ui, scene_arena, sys->fonts_view, &sys->renderer);

    SDL_StartTextInput(sys->window);

    slice_push_range(&menu.text, slice_str_literal("127.0.0.1"));

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



void text_field_input(String* text) {
    slice_push_range(text, input_get_ctx()->input_text);
    // string_cat(text, input_get_ctx()->input_text);

    if (input_modifier(SDL_KMOD_CTRL) && input_key_down_repeat(SDL_SCANCODE_V)) {
        String clipboard_view = cstr_to_string(SDL_GetClipboardText());
        slice_push_range(text, clipboard_view);
        // string_cat(text, cstr_to_string(SDL_GetClipboardText()));
    }

    if (input_modifier(SDL_KMOD_CTRL) && input_key_down_repeat(SDL_SCANCODE_BACKSPACE)) {
        i32 cut_to = -1;
        bool next_whitespace = false;
        for (i32 i = text->length - 1; i >= 0; (i--, cut_to = i)) {
            if (next_whitespace && slice_get(*text, i) == ' ') {
                break;
            } else if (slice_get(*text, i) != ' ') {
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

hashmap_def(u32, u32);

void menu_update(Menu* menu, Arena* temp_arena) {

    // if (menu->input->key_down(SDL_SCANCODE_S)) {
    //     f = !f;
    // }
    System* sys = menu->sys;

    ui_set_ctx(&menu->ui);

    ui_begin();

    ArenaTemp scratch = scratch_get(0, 0);
    defer_loop(0, scratch_release(scratch)) {


    if (input_key_down(SDL_SCANCODE_C)) {
        printf("aaaaan");
    }
    if (input_mouse_down(SDL_BUTTON_LEFT)) {
        printf("b");
    }
    ui_row({
        .font_size = font_px(32),
        .stack_axis = Axis2_Y,
        .position = pos_anchor2(0.5, 0.5),
        // .background_color = {0.1,0.1,0.3,1}
    }) {

        if (input_mouse_down(SDL_BUTTON_LEFT)) {
            int as = 123;
        }

        if (ui_button(ui_row_ret({
            .text = "Local",
        }))) {
            ring_push_back(&sys->events, (Event){
                .type = EventType_StartScene,
                .game_start = {
                    .online = false,
                },
            });
        }

        ui_row({0}) {
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
                ui_prev_element()->border_color = (float4){0.9,0.9,0.9,1};
                text_field_input(&menu->text);
            }


            // if (ui_is_active(ui_key(ip_field_key))) {
            // }

            if (ui_button(ui_row_ret({
                .text = "Connect",
                .background_color = {1,0,0,1},
            }))) {
                printf("%s\n", string_to_cstr(scratch.arena, menu->text));

                slice_push(&menu->text, (u8)'\0');
                ring_push_back(&sys->events, (Event){
                    .type = EventType_StartScene,
                    .game_start = {
                        .online = true,
                        .connect_ip = string8_literal((char*)menu->text.data),
                    },
                });
            }

        }
    }

    ui_end(temp_arena);

    ui_draw(&menu->ui, temp_arena);

    }
}


DLL_EXPORT Signals update(void* memory) {
    State* state = (State*)memory;

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

    f64 this_frame_time = os_now_seconds();
    f64 delta_time = this_frame_time - state->last_frame_time;
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
        Event union_event = ring_pop_front(&sys->events);
        
        switch (union_event.type) {
        case EventType_StartScene: {
            arena_reset(&state->level_arena);
            Slice_Font fonts_view = slice_create_view(Font, sys->fonts, array_length(sys->fonts));

            GameStartEvent event = union_event.game_start;            

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
        // auto render_begin_time = std::chrono::high_resolution_clock::now();
        begin_rendering(sys->window, &state->temp_arena);

        draw_screen_rect((Rect) {0,0,50,50}, (float4) {1,0,0,1});
        draw_screen_rect((Rect) {1000,0,24,50}, (float4) {1,0,0,1});

        ArenaTemp scratch = scratch_get(0, 0);
        Hashmap_u32_u32 hm = hashmap_alloc(u32, u32, scratch.arena, 10);
        hashmap_set(&hm, 32, 100);
        u32 ret = hashmap_get(hm, 32);

        scratch_release(scratch);


        draw_text(sys->fonts[0], "KYS!!!", 50, (float2){0,0}, rgba_white, 9999);


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

    return (Signals) {
        .quit = quit,
        .reload = reload,
    };
}
