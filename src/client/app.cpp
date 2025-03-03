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

struct Norm4 {
    float r, g, b, a;
};

RGBA norm4_to_rgba(Norm4 norm4) {
    return {
        .r = u8(norm4.r * 255),
        .g = u8(norm4.g * 255),
        .b = u8(norm4.b * 255),
        .a = u8(norm4.a * 255),
    };
}


// dt just to convert tick to time
// dt should be constant but paramaterizing it to allow different tick rate servers
// void render_state(Renderer* renderer, SDL_Window* window, GameState* state, int current_tick, double dt, Camera2D camera) {
//     using namespace renderer;
//
//     int idle_frame = current_tick % idle_cycle_period_ticks / idle_period_ticks;
//
//     for (i32 i = 0; i < state->entities.length; i++) {
//         const Entity* ent = &state->entities[i];
//
//         if (!ent->is_active) {
//             continue;
//         }
//
//         Rect world_rect = {};
//         TextureID texture;
//         switch (ent->type) {
//         case EntityType::Player: {
//             world_rect.size = {2,2};
//             texture = TextureID_player_png;
//         } break;
//         case EntityType::Bullet: {
//             world_rect.size = {0.5,0.5};
//             texture = TextureID_bullet_png;
//         } break;
//         case EntityType::Box: {
//             world_rect.size = {1,1};
//             texture = TextureID_box_png;
//         } break;
//
//         }
//
//         RGBA flash_color = {255,255,255,255};
//         f32 t = 0;
//         if (ent->flags & EntityFlags_hittable && ent->hit_flash_end_tick > current_tick) {
//             t = 1;
//         }
//
//         world_rect.position.b2vec = b2Body_GetPosition(ent->body_id);
//
//         if (ent->flip_sprite) {
//             world_rect.size.x *= -1;
//         }
//
//         draw_sprite_world(renderer, camera, world_rect, SpriteProperties{
//             .texture_id=texture,
//             .mix_color = flash_color,
//             .t = t,
//         });
//
//         if (ent->flags & EntityFlags_hittable) {
//             draw_world_rect(renderer, camera, {.position = world_rect.position - float2{0, -1}, .size={1, 0.1}}, {0.2, 0.2, 0.2, 1.0});
//             draw_world_rect( renderer, camera, {.position=world_rect.position - float2{0, -1}, .size={ent->health / (float)box_health, 0.1}}, {1, 0.2, 0.1, 1.0});
//         }
//     }
// }






// void begin_tick_input(input ) {
//
//     inpubutton_held_flags = SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
//     mod_state = SDL_GetModState();
// }

// class MultiplayerScene {
// public:
//     Input::Input& m_input;
//     Font* m_font;
//
//     double accumulator = 0.0;
//     uint32_t frame = {};
//     double time = {};
//     int current_tick = 0;
//
//     double speed_up_dt = 0.0;
//
//
//     UI ui;
//
//     Input::Input m_tick_input;
//
//     Camera2D m_camera = default_camera;
//
//     GameClient m_client;
//
//     MultiplayerScene(Input::Input& input, Font* font) : m_input(input), ui(font), m_font(font) {
//         m_tick_input.init_keybinds(Input::default_keybindings);
//     }
//
//     void connect() {
//         m_client.connect(yojimbo::Address(127, 0, 0, 1, 5000));
//     }
//
//     void update(double delta_time) {
//         using namespace Input;
//
//         accumulator += delta_time;
//         ImGui_ImplSDLRenderer3_NewFrame();
//         ImGui_ImplSDL3_NewFrame();
//         ImGui::NewFrame();
//
//         accumulate_input_events(m_tick_input, m_input);
//
//         while (accumulator >= fixed_dt) {
//             m_tick_input.begin_frame();
//             accumulator = accumulator - fixed_dt + speed_up_dt;
//             frame++;
//             time += fixed_dt;
//             // printf("frame:%d %fs\n", frame, time);
//
//
//
//             if (m_input.key_down(SDL_SCANCODE_R)) {
//                 accumulator += fixed_dt;
//             }
//
//             if (m_input.key_down(SDL_SCANCODE_E)) {
//                 accumulator -= fixed_dt;
//             }
//
//             auto player_input = input_state(m_tick_input, m_camera);
//
//
//             // printf("upate\n");
//
//             // ImGui_ImplSDLRenderer3_NewFrame();
//             // ImGui_ImplSDL3_NewFrame();
//             // ImGui::NewFrame();
//
//             int throttle_ticks{};
//             m_client.fixed_update(fixed_dt, player_input, m_input, &throttle_ticks);
//
//             speed_up_dt = throttle_ticks * fixed_dt * speed_up_fraction;
//
//             // accumulator += fixed_dt * throttle_ticks;
//
//             current_tick++;
//
//             m_tick_input.end_frame();
//         }
//
//         m_client.update();
//
//     }
//
//     void render(Renderer& renderer, SDL_Window* window) {
//         render_state(renderer, window, m_client.m_state, current_tick, fixed_dt, m_camera);
//
//         // renderer::draw_screen_rect(Renderer &renderer, Rect rect, RGBA rgba)
//
//         ImGui::Render();
//         ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), &renderer);
//     }
// };

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

    // (State&)memory = State{};
    Arena god_allocator{};
    arena_init(&god_allocator, memory, megabytes(16));
    State* state = (State*)arena_alloc(&god_allocator, sizeof(State));
    new (state) State{};

    arena_init(&state->temp_arena, arena_alloc(&god_allocator, megabytes(5)), megabytes(5));
    arena_init(&state->level_arena, arena_alloc(&god_allocator, megabytes(5)), megabytes(5));

    for (i32 i = 0; i < scratch_count; i++)  {
        scratch_arenas[i] = arena_suballoc(&god_allocator, megabytes(1));
    }


    // panic("asasdf {} {}", 234, 65345);

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

    state->input.init_keybinds(Input::default_keybindings);

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

    local_scene_init(&state->local_scene, &state->level_arena, &state->input, &state->renderer, fonts_view);
    // double last_render_duration = 0.0;


    state->initialized = true;
    return true;
}

// extern "C" INIT(Init) {
// }

extern "C" UPDATE(update) {
    auto state = (State*)memory;

    bool quit = false;

    if (!state->initialized) {
        if (!init(memory)) {
            quit = true;
        }
    }

    arena_reset(&state->temp_arena);


    auto this_frame_time = std::chrono::high_resolution_clock::now();
    double delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(this_frame_time - state->last_frame_time).count();
    state->last_frame_time = this_frame_time;

    state->input.begin_frame();

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
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.down) {
                break;
            }
            state->input.button_up_flags |= SDL_BUTTON_MASK(event.button.button);

        default:
            break;
        }
    }


    // if (state->input.key_down(SDL_SCANCODE_F)) {
    //     // ArenaTemp temp = scratch_get(0, 0);
    //     // defer(scratch_release(temp));
    //
    //
    //     Stream stream = {
    //         .slice = slice_create<u8>(&state->temp_arena, 512),
    //         .operation = Stream_Write,
    //     };
    //
    //
    //     TestMessage message = {
    //          .str = literal("KYS!"),
    //     };
    //     serialize_test_message(&stream, NULL, &message);
    //
    //     ENetPacket* packet = enet_packet_create(stream.slice.data, stream.current, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    //     enet_peer_send(state->server, Channel_Unreliable, packet);
    // }





    // if (state->input.key_down(SDL_SCANCODE_F) && state->input.modifier(SDL_KMOD_CTRL)) {
    //     printf("reset");
    //     state->initialized = false;
    //
    // }


    // update
    {
        // multi_scene.update(delta_time);
        local_scene_update(&state->local_scene, &state->temp_arena, delta_time);
        // state->local_scene.update(delta_time);
    }

    // render
    {
        auto render_begin_time = std::chrono::high_resolution_clock::now();
        renderer::begin_rendering(&state->renderer, state->window, &state->temp_arena);
        if (is_reloaded) {
            create_box(&state->local_scene.state, float2{2, 1});
            std::cout << "reloaded\n";

            // create_player(&state->local_scene.state);

            // SDL_SetWindowAlwaysOnTop(state->window, true);
        }

        // multi_scene.render(renderer, window, textures);
        local_scene_render(&state->local_scene, &state->renderer, &state->temp_arena, state->window);

        // //
        // // auto string = std::format("render: {:.3f}ms", last_render_duration * 1000.0);
        // // font::draw_text(renderer, font, string.data(), 24, {20, 30});
        // //

        renderer::end_rendering(&state->renderer);

        // last_render_duration = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() -
        // render_begin_time).count();
    }

    bool reload = false;
    if (state->input.key_down(SDL_SCANCODE_R) && state->input.modifier(SDL_KMOD_CTRL)) {
        reload = true;
    }

    state->input.end_frame();

    is_reloaded = false;

    return {
        .quit = quit,
        .reload = reload,
    };
}
