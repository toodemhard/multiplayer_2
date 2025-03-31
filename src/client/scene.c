#define GRID_STEP 1.0

Chunk chunks[4] = {
    {
        .position = {-chunk_width, chunk_width},
    },
    {
        .position = {0, chunk_width},
        .tiles = {{
            // .is_set = true,
            .texture = TextureID_tilemap_png,
            .x = 32,
            .w = 16,
            .h = 16,
        }}
    },
    {
        .position = {-chunk_width, 0}
    },
    {
        .position = {0, 0},
        .tiles = {{
            .is_set = true,
            .texture = TextureID_tilemap_png,
            .w = 16,
            .h = 16,
        }}
    },
};

void end_input_events(Input* tick_inputs) {
    zero_struct(&tick_inputs->keyboard_up);
    zero_struct(&tick_inputs->keyboard_down);
    zero_struct(&tick_inputs->keyboard_repeat);
    zero_struct(&tick_inputs->button_up_flags);
    zero_struct(&tick_inputs->button_down_flags);
}

void accumulate_input_events(Input* accumulator, Input* new_frame) {
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
        accumulator->keyboard_up[i] |= new_frame->keyboard_up[i];
        accumulator->keyboard_down[i] |= new_frame->keyboard_down[i];
        accumulator->keyboard_repeat[i] |= new_frame->keyboard_repeat[i];

        accumulator->button_up_flags |= new_frame->button_up_flags;
        accumulator->button_down_flags |= new_frame->button_down_flags;
    }
}

PlayerInput input_state(Camera2D camera, float2 window_size) {
    PlayerInput player_input = {0};

    if (input_action_held(ActionID_move_up)) {
        player_input.up = true;
    }
    if (input_action_held(ActionID_move_down)) {
        player_input.down = true;
    }
    if (input_action_held(ActionID_move_left)) {
        player_input.left = true;
    }
    if (input_action_held(ActionID_move_right)) {
        player_input.right = true;
    }

    if (input_action_down(ActionID_dash)) {
        player_input.dash = true;
    }


    for (i32 i = ActionID_hotbar_slot_1; i <= ActionID_hotbar_slot_8; i++) {
        if (input_action_down(i)) {
            player_input.select_spell[i - ActionID_hotbar_slot_1] = true;
        }
    }
    if (input_key_down(SDL_SCANCODE_2)) {
        player_input.select_spell[1] = true;
    }
    if (input_key_down(SDL_SCANCODE_3)) {
        player_input.select_spell[2] = true;
    }

    if (input_mouse_down(SDL_BUTTON_LEFT)) {
        player_input.fire = true;
    }

    player_input.cursor_world_pos = screen_to_world_pos(camera, input_mouse_position(), window_size.x, window_size.y);

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

slice_def(b2Vec2);

void DrawSolidPolygon(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context) {
    Renderer* renderer = (Renderer*)context;

    // auto asdf = hex_to_rgba(color);

    ArenaTemp scratch = scratch_get(0,0);
    defer_loop((void)0, scratch_release(scratch)) {

    Slice_b2Vec2 transed_verts = slice_create_fixed(b2Vec2, scratch.arena, vertexCount);

    for (int i = 0; i < vertexCount; i++) {
        b2Vec2 vert = vertices[i];

        b2Rot q = transform.q;
        float x = vert.x * q.c - vert.y * q.s;
        float y = vert.x * q.s + vert.y * q.c;

        vert.x = x;
        vert.y = y;

        vert.x += transform.p.x;
        vert.y += transform.p.y;

        *slice_getp(transed_verts, i) = vert;
    }

    draw_world_polygon(*renderer->active_camera, (float2*)transed_verts.data, vertexCount, hex_to_rgban(color));
    }
}

void DrawPolygon(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context) {
    Renderer* renderer = (Renderer*)context;

    // auto asdf = hex_to_rgba(color);

    draw_world_polygon(*renderer->active_camera, (float2*)vertices, vertexCount, hex_to_rgban(color));
}

void scene_render(Scene* s, Arena* frame_arena);

void scene_init(Scene* s, Arena* level_arena, System* sys, bool online, String8 ip_address) {
    zero_struct(s);
    // *s = {0};
    s->sys = sys;
    s->online_mode = online;

    s->camera = default_camera;
    sys->renderer.active_camera = &s->camera;

    arena_init(&s->tick_arena, arena_alloc(level_arena, megabytes(1)), megabytes(1));

    ui_init(&s->ui, level_arena, sys->fonts_view, &sys->renderer);

    state_init(&s->state, level_arena);

    s->player_handle = create_player(&s->state, 0);

    b2DebugDraw* m_debug_draw = &s->m_debug_draw;
    *m_debug_draw = b2DefaultDebugDraw();
    m_debug_draw->context = &sys->renderer;
    m_debug_draw->DrawPolygon = &DrawPolygon;
    m_debug_draw->DrawSolidPolygon = &DrawSolidPolygon;
    m_debug_draw->drawShapes = true;

    input_init(&s->tick_input, default_keybindings);

    // UI ui;
    // ui_init(&ui, level_arena);
    // ArenaTemp scratch = scratch_get(0,0);
    // for (ArenaTemp scratch = scratch_get(0,0);  scratch_release(scratch)) {
    //
    // }
    // ArenaTemp scratch = scratch_get(0,0); 
    // defer_loop(0, scratch_release(scratch)) {

    if (online) {
        s->client = enet_host_create(NULL, 1, 2, 0, 0);
        if (s->client == NULL) {
            fprintf (stderr, "An error occurred while trying to create an ENet client host.\n");
        }

        ENetAddress address;
        enet_address_set_host(&address, (char*)ip_address.data);
        address.port = 1234;
        s->server = enet_host_connect(s->client, &address, 2, 0);
        if (!s->server) {
            fprintf(stderr, "no peer");
        }

        // enet_peer_ping_interval(s->server, 10);
    }

    slice_init(&s->latest_snapshot, level_arena, 1000);

    // }
}

void scene_update(Scene* s, Arena* frame_arena, double delta_time) {
    // ZoneScoped;

    System* sys = s->sys;

    s->accumulator += delta_time;

    // s->tick_input.keybindings = sys->input.keybindings;
    array_copy(s->tick_input.keybindings, sys->input.keybindings);

    input_set_ctx(&sys->input);

    if (s->online_mode) {
        ENetEvent net_event;
        while (enet_host_service(s->client, &net_event, 0)) {
            switch (net_event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                printf ("A new client connected from %x:%u.\n", 
                    net_event.peer -> address.host,
                    net_event.peer -> address.port
                );

                /* Store any relevant client information here. */
                net_event.peer -> data = (void*)"Client information";

            } break;
            case ENET_EVENT_TYPE_RECEIVE: {
                // printf ("A packet of length %zu containing %s was received from %s on channel %u.\n",
                //     net_event.packet->dataLength,
                //     net_event.packet->data,
                //     (u8*)net_event.peer->data,
                //     net_event.channelID
                // );

                Stream stream = {
                    .slice = slice_create_view(u8, net_event.packet->data, net_event.packet->dataLength),
                    .operation = Stream_Read,
                };
                MessageType message_type;   // = *(MessageType*)net_event.packet->data;
                serialize_var(&stream, &message_type);
                stream_pos_reset(&stream);

                if (message_type == MessageType_Snapshot) {
                    // printf("asdl %f\n", delta_time);
                    // serialize_slice(&stream, &s->latest_snapshot);
                    u8 input_buffer_size;
                    serialize_snapshot(&stream, &input_buffer_size, &s->latest_snapshot, &s->player);
                    s->last_input_buffer_size = input_buffer_size;

                    s->camera.position = s->player.position;
                    f64 acc_diff = (target_input_buffer_size - input_buffer_size) / (f64)TICK_RATE * 0.05;
                    // printf("%f\n", acc_diff);
                    s->accumulator += acc_diff;
                }

                ArenaTemp scratch = scratch_get(0,0);
                defer_loop(0, scratch_release(scratch)) {

                    if (message_type == MessageType_Test) {
                        TestMessage message = {};
                        serialize_test_message(&stream, scratch.arena, &message);

                        printf("%s", slice_str_to_cstr(scratch.arena, message.str));
                    }

                }

                enet_packet_destroy (net_event.packet);
            } break;
            }
        }
    } else {

    }

    // Entity* player = entity_list_get(&s->state.entities, s->player_handle);
    // Entity zero = {};
    // Entity* player = &zero;



    if (input_key_down(SDL_SCANCODE_2)) {
        printf("alksdhfj\n");
    }

    // spell_move_source {}
    accumulate_input_events(&s->tick_input, &sys->input);

    ui_set_ctx(&s->ui);

    ui_begin();

    f32 grid_width = 4;
    f32 slot_width = 60;


    ui_push_row({
        .flags = UI_Flags_Float,
        .stack_axis = Axis2_X,
        .position = {position_offset_px(50), position_offset_px(50)},
    });


    for (i32 i = 0; i < 8; i++) {
        ImageID image = {0};
        float4 color = {1,1,1,1};
        if (s->player.selected_spell == i) {
            color = (float4){1,0.8,0,1};
        }
        UI_Key id = ui_push_row({
            .salt_key = var_to_byte_slice(&i),
            .size = {size_px(slot_width), size_px(slot_width)},
            .border = sides_px(grid_width),
            .background_color = {1,1,1,0.25},
            .border_color = color,
        });

        if (s->inventory_open) {
            if (ui_hover(id) && input_mouse_down(SDL_BUTTON_LEFT) && s->player.hotbar[i] != SpellType_NULL) {
                s->moving_spell = true;
                s->spell_move_src = i;
            }

            if (s->moving_spell && ui_hover(id) && input_mouse_up(SDL_BUTTON_LEFT)) {
                s->spell_move_dst = i;
                s->move_submit = true;
            }
        }

        SpellType spell = s->player.hotbar[i];
        Rect src = {0,0,16,16};
        if (spell != SpellType_NULL) {
            image = ImageID_spell_icons_png;

            if (spell == SpellType_Fireball) {
                src.position = (float2){16,0};
            }
            if (spell == SpellType_SniperRifle) {
                src.position = (float2){32,0};
            }
            if (spell == SpellType_SpreadBolt) {
                src.position = (float2){48,0};
            }
            if (spell == SpellType_IceWall) {
                src.position = (float2){0,0};
            }


            // if(i == 0) {
            //     image = ImageID_pot_jpg;
            // }
        }


        
        if (s->moving_spell && s->spell_move_src == i) {

            float2 pos = float2_sub(sys->input.mouse_pos, (float2){30,30});
            draw_sprite_screen((Rect){pos, {60,60}}, (SpriteProperties){
                .texture_id=image_id_to_texture_id(image),
                .src_rect = src,
            });
            image = 0;
        }

        ui_push_row({
            .image = image,
            .image_src = src,
            .size = {{UI_SizeType_ParentFraction, 1}, {UI_SizeType_ParentFraction, 1}},
            // .background_color = {1,0,0,1},
        });

        ui_pop_row();

        ui_pop_row();
    }
    
    ui_pop_row();

    // if (next_frame) {
    //     int x = 4123;
    //     next_frame = false;
    // }

    if (input_key_down(SDL_SCANCODE_TAB)) {
        s->inventory_open = !s->inventory_open;

    }

    if (input_key_down(SDL_SCANCODE_ESCAPE)) {
        s->paused = !s->paused;
    }


    /* Slice<u8> string = */ 
    String8 string = string8_format(frame_arena, "inptbuff: %d", s->last_input_buffer_size);
    // snprintf((char*)string.data, slice_size_bytes(string), );
    // snprintf((char*)string.data, slice_size_bytes(string), "inptbuff: %d", s->last_input_buffer_size);
    // printf("%s", string.data);


    if (s->online_mode) {
        s->acc_2 += delta_time;

        const f64 bw_poll = 1;

        if (s->acc_2 >= bw_poll) {
            s->acc_2  -= bw_poll;

            s->down_bandwidth = (s->client->totalReceivedData - s->last_total_received_data) / bw_poll;
            s->last_total_received_data = s->client->totalReceivedData;

            s->up_bandwidth = (s->client->totalSentData - s->last_total_sent_data) / bw_poll;
            s->last_total_sent_data = s->client->totalSentData;
        }

        ui_row({
            .stack_axis = Axis2_Y,
            .position=pos_anchor2(1,1),
            .padding=sides_px(ru(1)),
        }) {
            ui_row({
                .text=(char*)string.data,
            });
            ui_row({
                .text=(char*)string8_format(frame_arena, "ping: %dms", (int)s->server->lastRoundTripTime).data,
                // .background_color={1,0,0,1},
            });
            ui_row({
                .text=(char*)string8_format(frame_arena, "up: %.2fKB/s", s->up_bandwidth / 1024.0).data,
            });
            ui_row({
                .text=(char*)string8_format(frame_arena, "down: %.2fKB/s", s->down_bandwidth / 1024.0).data,
            });
            // printf("%d\n", s->client->incomingBandwidth);
        }
    }


    if (s->inventory_open) {
        ui_push_row({
            .flags = UI_Flags_Float,
            .stack_axis = Axis2_Y,
            .position = {position_offset_px(50), position_offset_px(150)},
        });

        for (i32 j = 0; j < 4; j++) {
            ui_push_row({
            });
            for (i32 i = 0; i < 8; i++) {
                ImageID image = {0};
                float4 color = {1,1,1,1};
                ui_push_row({
                    .size = {size_px(slot_width), size_px(slot_width)},
                    .border = sides_px(grid_width),
                    .border_color = color,
                    // .background_color = {1,1,1,1},
                });
                ui_row({
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

    if (s->paused) {
        ui_row({
            .font_size = font_ru(2),
            .size = {{UI_SizeType_ParentFraction, 1}, {UI_SizeType_ParentFraction, 1}},
            .background_color = {0,0,0,0.5},
        }) {
            ui_row({
                .position = pos_anchor2(0.5 , 0.5),
            }) {
                if (ui_button(ui_row_ret({
                    .text = "Quit"
                }))) {
                    // printf("fakjdhf\n");
                    if (s->online_mode) {
                        enet_peer_disconnect(s->server, 0);
                        enet_host_flush(s->client);
                        // enet_host_service(s->server) {
                        //
                        // }
                    }
                    ring_push_back(&sys->events, (Event){.type = EventType_QuitScene});
                }
            }
        }
    }

    // printf("%d", s->server->roundTripTime);
    // ui_push_row({
    //
    // })
    // ui_pop_row();


    ui_end(frame_arena);

    while (s->accumulator >= FIXED_DT) {
        arena_reset(&s->tick_arena);
        input_begin_frame(&s->tick_input);

        input_set_ctx(&s->tick_input);

        if (input_key_down(SDL_SCANCODE_2)) {
            printf("alksdhfj\n");
        }

        s->accumulator = s->accumulator - FIXED_DT;
        s->time += FIXED_DT;

        if (input_key_down(SDL_SCANCODE_R)) {
            s->accumulator += FIXED_DT;
        }

        if (input_key_down(SDL_SCANCODE_E)) {
            s->accumulator -= FIXED_DT;
        }

        float2 aa = input_mouse_position();
        if (input_mouse_down(SDL_BUTTON_LEFT)) {
            printf("%f, %f\n", aa.x, aa.y);
        }

        ClientID id = 0;
        PlayerInput input = input_state(s->camera, (float2){(f32)sys->renderer.window_width, (f32)sys->renderer.window_height});

        if (s->move_submit) {
            s-> move_submit = false;
            input.move_spell_src = s->spell_move_src;
            input.move_spell_dst = s->spell_move_dst;
            s->moving_spell = false;
        }

        Inputs inputs = {
            .ids = slice_create_view(ClientID, &id, 1),
            .inputs = slice_create_view(PlayerInput, &input, 1),
        };

        if (input_key_down(SDL_SCANCODE_F)) {
            Stream stream = {
                .slice = slice_create(u8, &s->tick_arena, kilobytes(2)),
                .operation = Stream_Write,
            };
            TestMessage msg = {
                .str = slice_str_literal("KYS!!!"),
            };
            serialize_test_message(&stream, NULL, &msg);
            ENetPacket* packet = enet_packet_create(stream.slice.data, stream.slice.length, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(s->server, Channel_Reliable, packet);
            enet_host_flush(s->client);
        }

        Stream stream = {
            .slice = slice_create(u8, &s->tick_arena, sizeof(PlayerInput)),
            .operation = Stream_Write,
        };


        if (s->online_mode) {
            // enet_peer_ping(s->server);
            serialize_input_message(&stream, &input);

            ENetPacket* packet = enet_packet_create(stream.slice.data, stream.slice.length, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(s->server, Channel_Reliable, packet);
            enet_host_flush(s->client);
        }


        if (input_mouse_up(SDL_BUTTON_LEFT)) {
            s->moving_spell = false;
        }
        // int throttle_ticks{};
        // state_update(&s->state, &tick_arena, inputs, s->current_tick, tick_rate);


        // float2 player_pos = {};
        // if (entity_is_valid(&s->state.entities, s->player_handle)) {
        //     player_pos = float2{.b2vec=b2Body_GetPosition(entity_list_get(&s->state.entities, s->player_handle)->body_id)};
        // }
        state_update(&s->state, &s->tick_arena, inputs, s->current_tick, TICK_RATE);

        if (!s->online_mode) {
            Slice_pEntity players = slice_p_create(Entity, &s->tick_arena, 1);
            s->latest_snapshot = entities_to_snapshot(&s->tick_arena, s->state.entities, s->current_tick, &players);
            s->player = *slice_get(players, 0);
            s->camera.position = s->player.position;
        }

        input_end_frame(&s->tick_input);
        s->current_tick++;

    }

    input_set_ctx(&sys->input);

    if (input_key_down(SDL_SCANCODE_E) && input_modifier(SDL_KMOD_CTRL)) {
        s->edit_mode = !s->edit_mode;
    }

    if (s->edit_mode) {
        if (input_mouse_held(SDL_BUTTON_LEFT)) {
            Chunk* chunk = &chunks[3];
            float2 pos = screen_to_world_pos(s->camera, sys->input.mouse_pos, sys->renderer.window_width, sys->renderer.window_height);
            i32 x = (i32) chunk->position.x + (floor(pos.x) / GRID_STEP);
            i32 y = (i32)chunk->position.y + (floor(pos.y) / GRID_STEP);
            Tile tile = {
                true,
                TextureID_tilemap_png,
                0,
                0,
                16,
                16,
            };

            i32 i = y * chunk_width + x;
            chunk->tiles[i] = tile;
            printf("%d, %d\n", x, y);
        }
    }

    s->frame++;

    scene_render(s, frame_arena);
}

bool float2_cmp(float2 a, float2 b) {
    return a.x == b.x && a.y == b.y;
}

void render_ghosts(Camera2D camera, Slice_Ghost ghosts) {
    for (i32 i = 0; i < ghosts.length; i++) {
        const Ghost* ghost = slice_getp(ghosts, i);

        Rect world_rect = {
            .position = ghost->position,
            .size = float2_scale(texture_dimensions(ghost->sprite), 1 / 16.0),
        };

        if (!float2_cmp(ghost->sprite_src.size, (float2){0,0})) {
            world_rect.size = float2_scale(ghost->sprite_src.size, 1 / 16.0);
        }
        // TextureID texture;
        // texture = ghost->sprite;
        // switch (ghost->type) {
        // case EntityType::Player: {
        //     world_rect.size = {2,2};
        //     texture = TextureID_player_png;
        // } break;
        // case EntityType::Bullet: {adf
        //     world_rect.size = {0.5,0.5};
        //     texture = TextureID_bullet_png;
        // } break;
        // case EntityType::Box: {
        //     world_rect.size = {1,1};
        //     texture = TextureID_box_png;
        // } break;
        //
        // }

        RGBA flash_color = {255,255,255,255};
        f32 t = 0;
        if (ghost->hit_flash) {
            t = 1;
        }


        if (ghost->flip_sprite) {
            world_rect.size.x *= -1;
        }

        draw_sprite_world(camera, world_rect, (SpriteProperties){
            .texture_id = ghost->sprite,
            .src_rect = ghost->sprite_src,
            .mix_color = flash_color,
            .t = t,
        });

        if (ghost->show_health) {
            float2 pos = float2_sub(world_rect.position, (float2){0, -1});
            draw_world_rect(camera, (Rect){
                    .position = pos,
                    .size={1, 0.1},
                },
                (float4){0.2, 0.2, 0.2, 1.0}
            );
            draw_world_rect(camera, (Rect){.position=pos, .size={ghost->health / (float)box_health, 0.1}}, (float4){1, 0.2, 0.1, 1.0});
        }
    }
}

void scene_render(Scene* s, Arena* frame_arena) {
    System* sys = s->sys;
    // printf("%d\n", s->latest_snapshot[0].type);
    draw_sprite_world(
        s->camera,
        (Rect){(float2){0,0}, {1,1}},
        (SpriteProperties){
            .texture_id = TextureID_ice_wall_png,

        }
        
    );
    render_ghosts(s->camera, s->latest_snapshot);
    for (i32 i = 0; i < 4; i++) {
        Chunk* chunk = &chunks[i];
        for (i32 tile_index = 0; tile_index < chunk_size; tile_index++) {
            Tile* tile = &chunk->tiles[tile_index];
            if (!tile->is_set) {
                continue;
            }
            float2 pos = float2_add(chunk->position, (float2){tile_index % chunk_width + 0.5f, (i32)(tile_index / chunk_width) + 0.5f});
            draw_sprite_world(
                s->camera,
                (Rect){.position = pos, .size={1, 1}},
                (SpriteProperties){
                    .texture_id = TextureID_tilemap_png,
                    .src_rect = (Rect){(float2){(f32)tile->x, (f32)tile->y}, {16,16}},
                }
            );
        }
    }

    if (!s->online_mode) {
        // b2World_Draw(s->state.world_id, &s->m_debug_draw);
    }
    // render_state(renderer, window, &s->state, s->current_tick, fixed_dt, s->camera);
    // render_ghosts(s->camera, s->latest_snapshot);
    // draw_world_rect(renderer, m_camera, {{-3.5, 0}, {1,1}}, color::red);
    // draw_world_rect(renderer, m_camera, {{0.5, 1.5}, {0.5,0.5}}, RGBA{255,255,0,255});
    // printf("%d\n", alignof(std::max_align_t);

    if (s->edit_mode) {
        i32 y_count = s->camera.size.y / GRID_STEP + 1;
        i32 x_count = s->camera.size.x / GRID_STEP + 1;
        float2 top_left_point = float2_add(s->camera.position, float2_scale((float2){-s->camera.size.x, s->camera.size.y}, 1 / 2.0));

        float4 grid_color = {0.2, 0.2, 0.2, 1};
        for (i32 y = 0; y < y_count; y++) {
            i32 start_y_index = (i32) (top_left_point.y / GRID_STEP);
            float2 left_point = (float2){top_left_point.x, (start_y_index - y) * GRID_STEP};
            float2 line[] = { left_point, float2_add(left_point, (float2){s->camera.size.x, 0}) };
            draw_world_lines(s->camera, line, 2, grid_color);
        }

        for (i32 x = 0; x < x_count; x++) {
            i32 start_x_index = (i32) (top_left_point.x / GRID_STEP);
            float2 top_point = (float2){(start_x_index + x) * GRID_STEP, top_left_point.y};
            float2 line[] = { top_point, float2_sub(top_point, (float2){0, s->camera.size.y}) };
            draw_world_lines(s->camera, line, 2, grid_color);
        }

        float2 pos = screen_to_world_pos(s->camera, sys->input.mouse_pos, sys->renderer.window_width, sys->renderer.window_height);
        

        i32 x = (i32) (floor(pos.x) / GRID_STEP);
        // if (pos.x < 0) {
        //     x -= 1;
        // }
        i32 y = (i32) (floor(pos.y) / GRID_STEP);
        // if (pos.y < 0) {
        //     y -= 1;
        // }

        float2 p2 = float2_add((float2){x * GRID_STEP, y * GRID_STEP}, float2_scale(float2_one, (GRID_STEP / 2)));
        draw_world_rect(s->camera, (Rect){p2,(float2){1,1}}, (float4){1,0,0,1});
    }

    ui_draw(&s->ui, frame_arena);
}
