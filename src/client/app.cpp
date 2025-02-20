#include "../pch.h"

#include "panic.h"
#include "renderer.h"
#include "stb_image.h"

#include "app.h"
#include "exports.h"

#include "common/game.h"


const int asdfhkj = 13413432;

constexpr double fixed_dt = 1.0 / tick_rate;
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
void render_state(Renderer* renderer, SDL_Window* window, GameState* state, int current_tick, double dt, Camera2D camera) {
    using namespace renderer;

    int idle_frame = current_tick % idle_cycle_period_ticks / idle_period_ticks;

    for (i32 i = 0; i < state->entities.length; i++) {
        const Entity* ent = &state->entities[i];

        if (!ent->is_active) {
            continue;
        }

        Rect world_rect = {};
        TextureID texture;
        switch (ent->entity_type) {
        case EntityType::Player: {
            world_rect.scale = {2,2};
            texture = TextureID::player_png;
        } break;
        case EntityType::Bullet: {
            world_rect.scale = {0.5,0.5};
            texture = TextureID::bullet_png;
        } break;
        case EntityType::Box: {
            world_rect.scale = {1,1};
            texture = TextureID::box_png;
        } break;

        }

        RGBA flash_color = {255,255,255,255};
        f32 t = 0;
        if (ent->flags & etbf(EntityComponent::hittable) && ent->hit_flash_end_tick > current_tick) {
            t = 1;
        }

        world_rect.position = b2vec_to_glmvec(b2Body_GetPosition(ent->body_id));

        if (ent->flip_sprite) {
            world_rect.scale.x *= -1;
        }

        draw_sprite_world(renderer, camera, world_rect, SpriteProperties{
            .texture_id=texture,
            .mix_color = flash_color,
            .t = t,
        });

        if (ent->flags & etbf(EntityComponent::hittable)) {
            draw_world_rect(renderer, camera, {world_rect.position - glm::vec2{0, -1}, {1, 0.1}}, norm4_to_rgba({0.2, 0.2, 0.2, 1.0}));
            draw_world_rect( renderer, camera, {world_rect.position - glm::vec2{0, -1}, {ent->health / (float)box_health, 0.1}}, norm4_to_rgba({1, 0.2, 0.1, 1.0}));
        }
    }
}

void accumulate_input_events(Input::Input& accumulator, Input::Input& new_frame) {
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
        accumulator.keyboard_up[i] |= new_frame.keyboard_up[i];
        accumulator.keyboard_down[i] |= new_frame.keyboard_down[i];
        accumulator.keyboard_repeat[i] |= new_frame.keyboard_repeat[i];

        accumulator.button_up_flags |= new_frame.button_up_flags;
        accumulator.button_down_flags |= new_frame.button_down_flags;
    }
}

PlayerInput input_state(Input::Input& input, const Camera2D& camera) {
    using namespace Input;
    PlayerInput player_input{};

    if (input.action_held(ActionID::move_up)) {
        player_input.up = true;
    }
    if (input.action_held(ActionID::move_down)) {
        player_input.down = true;
    }
    if (input.action_held(ActionID::move_left)) {
        player_input.left = true;
    }
    if (input.action_held(ActionID::move_right)) {
        player_input.right = true;
    }

    if (input.action_down(ActionID::dash)) {
        player_input.dash = true;
    }

    if (input.key_down(SDL_SCANCODE_F)) {
        printf("asdf\n");
    }

    if (input.mouse_down(SDL_BUTTON_LEFT) || input.key_down(SDL_SCANCODE_F)) {
        player_input.fire = true;
    }

    player_input.cursor_world_pos = screen_to_world_pos(camera, input.mouse_pos, window_width, window_height);

    return player_input;
}

RGBA hex_to_rgba(b2HexColor hex) {
    RGBA color;
    color.r = (hex >> 16) & 0xFF;
    color.g = (hex >> 8) & 0xFF;
    color.b = (hex & 0xFF);
    color.a = 200;

    return color;
}

void DrawSolidPolygon(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context) {
    auto& renderer = *(Renderer*)context;

    auto asdf = hex_to_rgba(color);

    std::vector<b2Vec2> transed_verts(vertexCount);

    for (int i = 0; i < vertexCount; i++) {
        auto vert = vertices[i];

        auto q = transform.q;
        float x = vert.x * q.c - vert.y * q.s;
        float y = vert.x * q.s + vert.y * q.c;

        vert.x = x;
        vert.y = y;

        vert.x += transform.p.x;
        vert.y += transform.p.y;

        transed_verts[i] = vert;
    }

    renderer::draw_world_polygon(&renderer, *renderer.active_camera, (glm::vec2*)transed_verts.data(), vertexCount, hex_to_rgba(color));
}

void DrawPolygon(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context) {
    auto& renderer = *(Renderer*)context;

    auto asdf = hex_to_rgba(color);

    renderer::draw_world_polygon(&renderer, *renderer.active_camera, (glm::vec2*)vertices, vertexCount, hex_to_rgba(color));
}


void local_scene_init(LocalScene* scene, Arena* level_arena, Input::Input* input, Renderer* renderer, Font* font) {
    *scene = {};
    scene->m_input = input;
    scene->m_font = font;
    scene->m_renderer = renderer;

    renderer->active_camera = &scene->m_camera;

    state_init(&scene->m_state, level_arena);

    scene->player_handle = create_player(&scene->m_state);

    scene->m_tick_input.init_keybinds(Input::default_keybindings);

    auto& m_debug_draw = scene->m_debug_draw;
    m_debug_draw = b2DefaultDebugDraw();
    m_debug_draw.context = renderer;
    m_debug_draw.DrawPolygon = &DrawPolygon;
    m_debug_draw.DrawSolidPolygon = &DrawSolidPolygon;
    m_debug_draw.drawShapes = true;
}

static Chunk chunks[4] = {
    Chunk{
        .position = {-chunk_width, chunk_width},
    },
    Chunk{
        .position = {0, chunk_width},
        .tiles = {Tile{
            .is_set = true,
                .texture = TextureID::tilemap_png,
                .x=32,
                .w = 16,
                .h = 16,
        }}
    },
    Chunk{
        .position = {-chunk_width, 0}
    },
    Chunk{
        .position = {0, 0},
        .tiles = {Tile{
            .is_set = true,
            .texture = TextureID::tilemap_png,
            .w = 16,
            .h = 16,
        }}
    },
};

const static float grid_step = 1;

void local_scene_update(LocalScene* s, Arena* frame_arena, double delta_time) {
    ZoneScoped;

    s->accumulator += delta_time;
    accumulate_input_events(s->m_tick_input, *s->m_input);

    while (s->accumulator >= fixed_dt) {
        s->m_tick_input.begin_frame();
        s->accumulator = s->accumulator - fixed_dt;
        s->frame++;
        s->time += fixed_dt;
        // printf("frame:%d %fs\n", frame, time);

        if (s->m_input->key_down(SDL_SCANCODE_R)) {
            s->accumulator += fixed_dt;
        }

        if (s->m_input->key_down(SDL_SCANCODE_E)) {
            s->accumulator -= fixed_dt;
        }

        s->inputs[0] = input_state(s->m_tick_input, s->m_camera);

        int throttle_ticks{};
        state_update(&s->m_state, frame_arena, s->inputs, s->current_tick, tick_rate);

        auto player_pos = b2vec_to_glmvec(b2Body_GetPosition(entity_list_get(&s->m_state.entities, s->player_handle)->body_id));
        s->m_camera.position = player_pos;

        s->current_tick++;

        s->m_tick_input.end_frame();
    }

    if (s->m_input->key_down(SDL_SCANCODE_E) && s->m_input->modifier(SDL_KMOD_CTRL)) {
        s->m_edit_mode = !s->m_edit_mode;
    }

    if (s->m_edit_mode) {
        if (s->m_input->mouse_held(SDL_BUTTON_LEFT)) {
            auto& chunk = chunks[3];
            auto pos = screen_to_world_pos(s->m_camera, s->m_input->mouse_pos, s->m_renderer->window_width, s->m_renderer->window_height);
            i32 x = (i32) chunk.position.x + (floor(pos.x) / grid_step);
            i32 y = (i32)chunk.position.y + (floor(pos.y) / grid_step);
            Tile tile = {
                true,
                TextureID::tilemap_png,
                0,
                0,
                16,
                16,
            };

            i32 i = y * chunk_width + x;
            chunk.tiles[i] = tile;
            printf("%d, %d\n", x, y);
        }
    }
}

void local_scene_render(LocalScene* s, Renderer* renderer, SDL_Window* window) {
    for (i32 i = 0; i < 4; i++) {
        auto& chunk = chunks[i];
        for (i32 tile_index = 0; tile_index < chunk_size; tile_index++) {
            auto& tile = chunk.tiles[tile_index];
            if (!tile.is_set) {
                continue;
            }
            renderer::draw_sprite_world(
                renderer,
                s->m_camera,
                {chunk.position + glm::vec2{tile_index % chunk_width + 0.5, i32(tile_index / chunk_width) + 0.5}, {1, 1}},
                {
                    .texture_id = TextureID::tilemap_png,
                    .src_rect = Rect{{tile.x, tile.y}, {16,16}},
                }
            );
        }
    }

    render_state(renderer, window, &s->m_state, s->current_tick, fixed_dt, s->m_camera);
    // renderer::draw_world_rect(renderer, m_camera, {{-3.5, 0}, {1,1}}, color::red);
    // renderer::draw_world_rect(renderer, m_camera, {{0.5, 1.5}, {0.5,0.5}}, RGBA{255,255,0,255});
    // printf("%d\n", alignof(std::max_align_t);
    if (is_reloaded) {
        auto p = snap_pos({1,1});
        printf("%f, %f", p.x, p.y);
    }

    {
        glm::vec2 line[] = {{-1,2},{0,2},{0,3}};
        renderer::draw_world_lines(renderer, s->m_camera, line, 3, RGBA{255,255,0,255});
    }
    {
        glm::vec2 line[] = {snap_pos({-1,5}), snap_pos({-1,4}), snap_pos({0,1})};
        renderer::draw_world_lines(renderer, s->m_camera, line, 2, RGBA{255,0,0,255});
    }


    if (s->m_edit_mode) {
        i32 y_count = s->m_camera.scale.y / grid_step + 1;
        i32 x_count = s->m_camera.scale.x / grid_step + 1;
        glm::vec2 top_left_point = s->m_camera.position + glm::vec2{-s->m_camera.scale.x, s->m_camera.scale.y} / 2.0f;

        RGBA grid_color = {255, 255, 255, 160};
        for (i32 y = 0; y < y_count; y++) {
            i32 start_y_index = (i32) (top_left_point.y / grid_step);
            glm::vec2 left_point = glm::vec2{top_left_point.x, (start_y_index - y) * grid_step};
            glm::vec2 line[] = { left_point, left_point + glm::vec2{s->m_camera.scale.x, 0} };
            renderer::draw_world_lines(renderer, s->m_camera, line, 2, grid_color);
        }

        for (i32 x = 0; x < x_count; x++) {
            i32 start_x_index = (i32) (top_left_point.x / grid_step);
            glm::vec2 top_point = glm::vec2{(start_x_index + x) * grid_step, top_left_point.y};
            glm::vec2 line[] = { top_point, top_point - glm::vec2{0, s->m_camera.scale.y} };
            renderer::draw_world_lines(renderer, s->m_camera, line, 2, grid_color);
        }

        auto pos = screen_to_world_pos(s->m_camera, s->m_input->mouse_pos, renderer->window_width, renderer->window_height);
        

        i32 x = (i32) (floor(pos.x) / grid_step);
        // if (pos.x < 0) {
        //     x -= 1;
        // }
        i32 y = (i32) (floor(pos.y) / grid_step);
        // if (pos.y < 0) {
        //     y -= 1;
        // }

        glm::vec2 p2 = glm::vec2{x * grid_step, y * grid_step} + (grid_step / 2);
        glm::vec2 kys[] = {{0,0}, {1,0}};
        renderer::draw_world_rect(renderer, s->m_camera, {p2,{1,1}}, color::red);

    }

    // renderer::draw_world_sprite(renderer, m_camera, {{0,0}, {1,1}}, {
    //     .texture_id=TextureID::tilemap_png,
    //     .src_rect=Rect{{16,0}, {16,16}}
    // });
    
    // renderer::draw_world_lines(renderer, m_camera, kys, 2, {255,255,0,255});
    renderer::draw_sprite_world(renderer, s->m_camera, {{0,1}, {1,1}}, {
        .texture_id = TextureID::depth_test_png,
    });



    // auto rect = renderer::world_rect_to_normalized(m_camera, {{1,1}, {1,1}});
    // renderer::draw_textured_rect(renderer, TextureID::amogus_png, rect, {});

    b2World_Draw(s->m_state.world_id, &s->m_debug_draw);

    glm::vec2 idk[] = {{-1, 1}, {1, 1}, {1, -1}, {-1, -1}};
    // renderer::draw_world_polygon(renderer, m_camera, idk, 4, color::red);

}

void poll_event() {}

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

extern "C" INIT(init) {
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
    State* state = (State*)arena_allocate(&god_allocator, sizeof(State));
    new (state) State{};

    arena_init(&state->temp_arena, arena_allocate(&god_allocator, megabytes(5)), megabytes(5));
    arena_init(&state->level_arena, arena_allocate(&god_allocator, megabytes(5)), megabytes(5));


    // panic("asasdf {} {}", 234, 65345);

    auto flags = SDL_WINDOW_INPUT_FOCUS; // | SDL_WINDOW_ALWAYS_ON_TOP; // | SDL_WINDOW_FULLSCREEN;

    state->window = SDL_CreateWindow("ye", window_width, window_height, flags);
    if (state->window == NULL) {
        SDL_Log("CreateWindow failed: %s", SDL_GetError());
    }

    if (init_renderer(&state->renderer, state->window) != 0) {
        panic("failed to initialize renderer");
    }

    // Texture textures[(int)ImageID::image_count];
    std::future<void> texture_futures[(int)ImageID::image_count];

    auto image_to_texture = [](Renderer& renderer, ImageID image_id) {
        int width, height, comp;
        unsigned char* image = stbi_load(image_paths[(int)image_id], &width, &height, &comp, STBI_rgb_alpha);
        renderer::load_texture(&renderer, image_id_to_texture_id(image_id), Image{.w = width, .h = height, .data = image});
    };

    for (int i = 0; i < (int)ImageID::image_count; i++) {
        texture_futures[i] = std::async(std::launch::async, image_to_texture, std::ref(state->renderer), (ImageID)i);
    }

    Font font;
    auto font_future =
        std::async(std::launch::async, font::load_font, &font, std::ref(state->renderer), FontID::Avenir_LT_Std_95_Black_ttf, 512, 512, 64);

    state->input.init_keybinds(Input::default_keybindings);

    glm::vec2 player_position(0, 0);

    // state->local_scene.frame = 12312;
    // (&state->local_scene)->m_input = &state->input;




    local_scene_init(&state->local_scene, &state->level_arena, &state->input, &state->renderer, &font);

    InitializeYojimbo();

    for (int i = 0; i < (int)ImageID::image_count; i++) {
        texture_futures[i].wait();
    }
    font_future.wait();

    state->last_frame_time = std::chrono::high_resolution_clock::now();
    // double last_render_duration = 0.0;

    // tick is update
    // frame is animation keyframe

    // MultiplayerScene multi_scene(input, &font);
    // multi_scene.connect();

    state->rects = {{255, 0, 0, 255}, {255, 255, 0, 255}, {0, 255, 255, 255}};
    //

}

extern "C" UPDATE(update) {
    auto state = (State*)memory;

    arena_reset(&state->temp_arena);

    // kys();

    bool quit = false;

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

    if(state->input.mouse_down(0)) {
       printf("sdkhfsd\n");
    }

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
            // create_box(state->local_scene.m_state, glm::vec2{2, 1});
            std::cout << "reloaded\n";

            // SDL_SetWindowAlwaysOnTop(state->window, true);
        }

        // multi_scene.render(renderer, window, textures);
        local_scene_render(&state->local_scene, &state->renderer, state->window);

        // //
        // // auto string = std::format("render: {:.3f}ms", last_render_duration * 1000.0);
        // // font::draw_text(renderer, font, string.data(), 24, {20, 30});
        // //

        renderer::end_rendering(&state->renderer);

        

        // last_render_duration = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() -
        // render_begin_time).count();
    }

    state->input.end_frame();

    is_reloaded = false;

    return {
        .quit = quit,
    };
}
