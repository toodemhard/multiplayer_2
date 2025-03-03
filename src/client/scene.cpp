#include "pch.h"

#include "scene.h"
#include "common/net_common.h"

const static float grid_step = 1;

static Chunk chunks[4] = {
    Chunk{
        .position = {-chunk_width, chunk_width},
    },
    Chunk{
        .position = {0, chunk_width},
        .tiles = {Tile{
            .is_set = true,
                .texture = TextureID_tilemap_png,
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
            .texture = TextureID_tilemap_png,
            .w = 16,
            .h = 16,
        }}
    },
};

void end_input_events(Input::Input* tick_inputs) {
    tick_inputs->keyboard_up = {};
    tick_inputs->keyboard_down = {};
    tick_inputs->keyboard_repeat = {};
    tick_inputs->button_up_flags = {};
    tick_inputs->button_down_flags = {};
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

PlayerInput input_state(Input::Input& input, const Camera2D& camera, float2 window_size) {
    using namespace Input;
    PlayerInput player_input{};

    if (input.action_held(ActionID_move_up)) {
        player_input.up = true;
    }
    if (input.action_held(ActionID_move_down)) {
        player_input.down = true;
    }
    if (input.action_held(ActionID_move_left)) {
        player_input.left = true;
    }
    if (input.action_held(ActionID_move_right)) {
        player_input.right = true;
    }

    if (input.action_down(ActionID_dash)) {
        player_input.dash = true;
    }


    for (i32 i = ActionID_hotbar_slot_1; i <= ActionID_hotbar_slot_8; i++) {
        if (input.action_down(i)) {
            player_input.select_spell[i - ActionID_hotbar_slot_1] = true;
        }
    }
    if (input.key_down(SDL_SCANCODE_2)) {
        player_input.select_spell[1] = true;
    }
    if (input.key_down(SDL_SCANCODE_3)) {
        player_input.select_spell[2] = true;
    }

    if (input.mouse_down(SDL_BUTTON_LEFT)) {
        player_input.fire = true;
    }

    player_input.cursor_world_pos = screen_to_world_pos(camera, input.mouse_pos, window_size.x, window_size.y);

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

float4 hex_to_rgban(b2HexColor hex) {
    float4 color;
    color.r = ((hex >> 16) & 0xFF) / 255.0f;
    color.g = ((hex >> 8) & 0xFF) / 255.0f;
    color.b = ((hex & 0xFF)) / 255.0f;
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

    renderer::draw_world_polygon(&renderer, *renderer.active_camera, (float2*)transed_verts.data(), vertexCount, hex_to_rgban(color));
}

void DrawPolygon(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context) {
    auto& renderer = *(Renderer*)context;

    auto asdf = hex_to_rgba(color);

    renderer::draw_world_polygon(&renderer, *renderer.active_camera, (float2*)vertices, vertexCount, hex_to_rgban(color));
}

void local_scene_init(LocalScene* scene, Arena* level_arena, Input::Input* input, Renderer* renderer, Slice<Font> fonts) {
    *scene = {};
    scene->input = input;
    scene->renderer = renderer;

    scene->fonts = fonts;

    renderer->active_camera = &scene->camera;

    ui_init(&scene->ui, level_arena, fonts, renderer);

    state_init(&scene->state, level_arena);

    scene->player_handle = create_player(&scene->state, 0);

    auto& m_debug_draw = scene->m_debug_draw;
    m_debug_draw = b2DefaultDebugDraw();
    m_debug_draw.context = renderer;
    m_debug_draw.DrawPolygon = &DrawPolygon;
    m_debug_draw.DrawSolidPolygon = &DrawSolidPolygon;
    m_debug_draw.drawShapes = true;

    // UI ui;
    // ui_init(&ui, level_arena);

    scene->client = enet_host_create(NULL, 1, 2, 0, 0);
    if (scene->client == NULL) {
        fprintf (stderr, "An error occurred while trying to create an ENet client host.\n");
    }

    slice_init(&scene->latest_snapshot, level_arena, 1000);

    ENetAddress address;
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 1234;
    scene->server = enet_host_connect(scene->client, &address, 2, 0);
    if (!scene->server) {
        fprintf(stderr, "no peer");
    }
}

void local_scene_update(LocalScene* s, Arena* frame_arena, double delta_time) {
    ZoneScoped;

    Arena tick_arena;
    arena_init(&tick_arena, arena_alloc(frame_arena, megabytes(1)), megabytes(1));

    s->accumulator += delta_time;

    s->tick_input.keybindings = s->input->keybindings;

    ENetEvent net_event;
    while (enet_host_service(s->client, &net_event, 0)) {
        switch (net_event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            printf ("A new client connected from %x:%u.\n", 
                net_event.peer -> address.host,
                net_event.peer -> address.port
            );

            /* Store any relevant client information here. */
            net_event.peer -> data = (void*)"Client information";

            break;
        case ENET_EVENT_TYPE_RECEIVE: {
            // printf ("A packet of length %zu containing %s was received from %s on channel %u.\n",
            //     net_event.packet->dataLength,
            //     net_event.packet->data,
            //     (u8*)net_event.peer->data,
            //     net_event.channelID
            // );

            Stream stream = {
                .slice = slice_create_view(net_event.packet->data, net_event.packet->dataLength),
                .operation = Stream_Read,
            };
            MessageType message_type;   // = *(MessageType*)net_event.packet->data;
            serialize_var(&stream, &message_type);
            stream_pos_reset(&stream);

            if (message_type == MessageType_Snapshot) {
                printf("asdl %f\n", delta_time);
                // serialize_slice(&stream, &s->latest_snapshot);
                u8 input_buffer_size;
                serialize_snapshot(&stream, &input_buffer_size, &s->latest_snapshot);

                s->accumulator += (target_input_buffer_size - input_buffer_size) / (f64)tick_rate * 0.01;
            }

            enet_packet_destroy (net_event.packet);
        } break;
        }
    }

    Entity* player = entity_list_get(&s->state.entities, s->player_handle);



    // spell_move_source {}
    accumulate_input_events(s->tick_input, *s->input);

    ui_set_ctx(&s->ui);

    ui_begin(s->input);

    f32 grid_width = 4;
    f32 slot_width = 60;


    ui_push_row({
        .flags = UI_Flags_Float,
        // .grow_axis = Axis2_Y,
        .position = {position_offset_px(50), position_offset_px(50)},
    });

    for (i32 i = 0; i < 8; i++) {
        ImageID image = {};
        float4 color = {1,1,1,1};
        if (player->selected_spell == i) {
            color = {1,0.8,0,1};
        }
        UI_Key id = ui_push_row({
            .size = {size_px(slot_width), size_px(slot_width)},
            .border = sides_px(grid_width),
            // .background_color = {1,1,1,1},
            .border_color = color,
        });

        if (s->inventory_open) {
            if (ui_hover(id) && s->input->mouse_down(SDL_BUTTON_LEFT) && player->hotbar[i] != SpellType_NULL) {
                s->moving_spell = true;
                s->spell_move_src = i;
            }

            if (s->moving_spell && ui_hover(id) && s->input->mouse_up(SDL_BUTTON_LEFT)) {
                s->spell_move_dst = i;
                s->move_submit = true;
            }
        }

        if (player->hotbar[i] != SpellType_NULL) {
            image = ImageID_bullet_png;

            // if(i == 0) {
            //     image = ImageID_pot_jpg;
            // }
        }
        
        if (s->moving_spell && s->spell_move_src == i) {
            image = {};
        }

        ui_push_leaf({
            .image = image,
            .size = {{UI_SizeType_ParentFraction, 1}, {UI_SizeType_ParentFraction, 1}},
            // .background_color = {1,0,0,1},
        });

        ui_pop_row();
    }
    
    ui_pop_row();

    // if (next_frame) {
    //     int x = 4123;
    //     next_frame = false;
    // }

    if (s->input->key_down(SDL_SCANCODE_TAB)) {
        s->inventory_open = !s->inventory_open;

    }



    if (s->inventory_open) {
        ui_push_row({
            .flags = UI_Flags_Float,
            .grow_axis = Axis2_Y,
            .position = {position_offset_px(50), position_offset_px(150)},
        });

        for (i32 j = 0; j < 4; j++) {
            ui_push_row({
            });
            for (i32 i = 0; i < 8; i++) {
                ImageID image = {};
                float4 color = {1,1,1,1};
                ui_push_row({
                    .size = {size_px(slot_width), size_px(slot_width)},
                    .border = sides_px(grid_width),
                    // .background_color = {1,1,1,1},
                    .border_color = color,
                });
                ui_push_leaf({
                    .image = image,
                    .size = {{UI_SizeType_ParentFraction, 0.5}, {UI_SizeType_ParentFraction, 0.5}},
                    // .background_color = {1,0,0,1},
                });
                ui_pop_row();
            }
            ui_pop_row();
        }
        ui_pop_row();
    }

    // printf("%d", s->server->roundTripTime);
    // ui_push_row({
    //
    // })
    // ui_pop_row();


    ui_end(frame_arena);

    while (s->accumulator >= fixed_dt) {
        s->tick_input.begin_frame();

        s->accumulator = s->accumulator - fixed_dt;
        s->time += fixed_dt;

        if (s->tick_input.key_down(SDL_SCANCODE_R)) {
            s->accumulator += fixed_dt;
        }

        if (s->tick_input.key_down(SDL_SCANCODE_E)) {
            s->accumulator -= fixed_dt;
        }

        ClientID id = 0;
        PlayerInput input = input_state(s->tick_input, s->camera, {(f32)s->renderer->window_width, (f32)s->renderer->window_height});

        if (s->move_submit) {
            s-> move_submit = false;
            input.move_spell_src = s->spell_move_src;
            input.move_spell_dst = s->spell_move_dst;
            s->moving_spell = false;
        }

        Inputs inputs = {
            .ids = slice_create_view(&id, 1),
            .inputs = slice_create_view(&input, 1),
        };

        Stream stream = {
            .slice = slice_create<u8>(&tick_arena, sizeof(PlayerInput)),
            .operation = Stream_Write,
        };

        serialize_input_message(&stream, &input);

        ENetPacket* packet = enet_packet_create(stream.slice.data, stream.slice.length, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(s->server, Channel_Reliable, packet);
        enet_host_flush(s->client);


        if (s->tick_input.mouse_up(SDL_BUTTON_LEFT)) {
            s->moving_spell = false;
        }
        // int throttle_ticks{};
        // state_update(&s->state, &tick_arena, inputs, s->current_tick, tick_rate);


        // float2 player_pos = {};
        // if (entity_is_valid(&s->state.entities, s->player_handle)) {
        //     player_pos = float2{.b2vec=b2Body_GetPosition(entity_list_get(&s->state.entities, s->player_handle)->body_id)};
        // }
        // s->camera.position = player_pos;

        s->current_tick++;

        s->tick_input.end_frame();

        arena_reset(&tick_arena);
    }

    if (s->input->key_down(SDL_SCANCODE_E) && s->input->modifier(SDL_KMOD_CTRL)) {
        s->edit_mode = !s->edit_mode;
    }

    if (s->edit_mode) {
        if (s->input->mouse_held(SDL_BUTTON_LEFT)) {
            auto& chunk = chunks[3];
            auto pos = screen_to_world_pos(s->camera, s->input->mouse_pos, s->renderer->window_width, s->renderer->window_height);
            i32 x = (i32) chunk.position.x + (floor(pos.x) / grid_step);
            i32 y = (i32)chunk.position.y + (floor(pos.y) / grid_step);
            Tile tile = {
                true,
                TextureID_tilemap_png,
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

    s->frame++;
}

void render_ghosts(Renderer* renderer, Camera2D camera, const Slice<Ghost> ghosts) {
    using namespace renderer;
    for (i32 i = 0; i < ghosts.length; i++) {
        const Ghost* ghost = &ghosts[i];

        Rect world_rect = {};
        TextureID texture;
        switch (ghost->type) {
        case EntityType::Player: {
            world_rect.size = {2,2};
            texture = TextureID_player_png;
        } break;
        case EntityType::Bullet: {
            world_rect.size = {0.5,0.5};
            texture = TextureID_bullet_png;
        } break;
        case EntityType::Box: {
            world_rect.size = {1,1};
            texture = TextureID_box_png;
        } break;

        }

        RGBA flash_color = {255,255,255,255};
        f32 t = 0;
        if (ghost->hit_flash) {
            t = 1;
        }

        world_rect.position = ghost->position;

        if (ghost->flip_sprite) {
            world_rect.size.x *= -1;
        }

        draw_sprite_world(renderer, camera, world_rect, SpriteProperties{
            .texture_id=texture,
            .mix_color = flash_color,
            .t = t,
        });

        if (ghost->show_health) {
            draw_world_rect(renderer, camera, {.position = world_rect.position - float2{0, -1}, .size={1, 0.1}}, {0.2, 0.2, 0.2, 1.0});
            draw_world_rect( renderer, camera, {.position=world_rect.position - float2{0, -1}, .size={ghost->health / (float)box_health, 0.1}}, {1, 0.2, 0.1, 1.0});
        }
    }
}

void local_scene_render(LocalScene* s, Renderer* renderer, Arena* frame_arena, SDL_Window* window) {
    // printf("%d\n", s->latest_snapshot[0].type);
    render_ghosts(renderer, s->camera, s->latest_snapshot);
    for (i32 i = 0; i < 4; i++) {
        auto& chunk = chunks[i];
        for (i32 tile_index = 0; tile_index < chunk_size; tile_index++) {
            auto& tile = chunk.tiles[tile_index];
            if (!tile.is_set) {
                continue;
            }
            renderer::draw_sprite_world(
                renderer,
                s->camera,
                {.position = chunk.position + float2{tile_index % chunk_width + 0.5f, i32(tile_index / chunk_width) + 0.5f}, .size={1, 1}},
                {
                    .texture_id = TextureID_tilemap_png,
                    .src_rect = Rect{float2{(f32)tile.x, (f32)tile.y}, {16,16}},
                }
            );
        }
    }

    // render_state(renderer, window, &s->state, s->current_tick, fixed_dt, s->camera);
    render_ghosts(renderer, s->camera, s->latest_snapshot);
    // renderer::draw_world_rect(renderer, m_camera, {{-3.5, 0}, {1,1}}, color::red);
    // renderer::draw_world_rect(renderer, m_camera, {{0.5, 1.5}, {0.5,0.5}}, RGBA{255,255,0,255});
    // printf("%d\n", alignof(std::max_align_t);
    if (s->moving_spell) {
        Entity* player = entity_list_get(&s->state.entities, s->player_handle);
        renderer::draw_sprite_screen(renderer, {s->input->mouse_pos - float2{30,30}, {60,60}}, {.texture_id=TextureID_bullet_png});
    }

    if (s->edit_mode) {
        i32 y_count = s->camera.scale.y / grid_step + 1;
        i32 x_count = s->camera.scale.x / grid_step + 1;
        float2 top_left_point = s->camera.position + float2{-s->camera.scale.x, s->camera.scale.y} / 2.0f;

        float4 grid_color = {0.2, 0.2, 0.2, 1};
        for (i32 y = 0; y < y_count; y++) {
            i32 start_y_index = (i32) (top_left_point.y / grid_step);
            float2 left_point = float2{top_left_point.x, (start_y_index - y) * grid_step};
            float2 line[] = { left_point, left_point + float2{s->camera.scale.x, 0} };
            renderer::draw_world_lines(renderer, s->camera, line, 2, grid_color);
        }

        for (i32 x = 0; x < x_count; x++) {
            i32 start_x_index = (i32) (top_left_point.x / grid_step);
            float2 top_point = float2{(start_x_index + x) * grid_step, top_left_point.y};
            float2 line[] = { top_point, top_point - float2{0, s->camera.scale.y} };
            renderer::draw_world_lines(renderer, s->camera, line, 2, grid_color);
        }

        auto pos = screen_to_world_pos(s->camera, s->input->mouse_pos, renderer->window_width, renderer->window_height);
        

        i32 x = (i32) (floor(pos.x) / grid_step);
        // if (pos.x < 0) {
        //     x -= 1;
        // }
        i32 y = (i32) (floor(pos.y) / grid_step);
        // if (pos.y < 0) {
        //     y -= 1;
        // }

        float2 p2 = float2{x * grid_step, y * grid_step} + (grid_step / 2) * float2_one;
        renderer::draw_world_rect(renderer, s->camera, Rect{(float2)p2,{1,1}}, {1,0,0,1});

    }

    // renderer::draw_world_sprite(renderer, m_camera, {{0,0}, {1,1}}, {
    //     .texture_id=TextureID_tilemap_png,
    //     .src_rect=Rect{{16,0}, {16,16}}
    // });
    
    // renderer::draw_world_lines(renderer, m_camera, kys, 2, {255,255,0,255});
    renderer::draw_sprite_world(renderer, s->camera, {float2{0,1}, {1,1}}, {
        .texture_id = TextureID_depth_test_png,
    });



    using namespace renderer;

    ui_draw(&s->ui, renderer, frame_arena);
}
