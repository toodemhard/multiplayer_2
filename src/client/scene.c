#define GRID_STEP 1.0

// Chunk chunks[4] = {
//     {
//         .position = {-chunk_width, chunk_width},
//     },
//     {
//         .position = {0, chunk_width},
//         .tiles = {{
//             // .is_set = true,
//             .texture = TextureID_tilemap_png,
//             .x = 32,
//             .w = 16,
//             .h = 16,
//         }}
//     },
//     {
//         .position = {-chunk_width, 0}
//     },
//     {
//         .position = {0, 0},
//         .tiles = {{
//             .is_set = true,
//             .texture = TextureID_tilemap_png,
//             .w = 16,
//             .h = 16,
//         }}
//     },
// };

// typedef enum GameEventType {
//     GameEventType_CreateEntity,
//     GameEventType_DeleteEntity,
// } GameEventType;
//
//
// typedef struct GameEvent {
//     GameEventType type;
//     union {
//         DeleteEntityEvent delete_entity;
//         CreateEntityEvent create_entity;
//     };
// } GameEvent;

// void tick_queue_init(TickQueue* queue, Arena* arena, u64 size) {
//     queue->buffer = arena_alloc(arena, size);
//     queue->capacity = size;
// }
//
// #define move_slice_into_stream(stream, _slice)\
// do {\
//     u64 size = slice_size(*(_slice));\
//     u8* data = (u8*)slice_getp((stream)->slice, (stream)->current);\
//     serialize_bytes((stream), data, size);\
//     *((u8**)&(_slice)->data) = data;\
// } while(0)
//
// // copies data reffed in packet
// void queue_tick(TickQueue* queue, Tick tick) {
//     u64 chunk_size = sizeof(tick) + slice_size(tick.entities) + slice_size(tick.create_events) + slice_size(tick.delete_events);
//     tick.total_size = chunk_size;
//
//     ASSERT(queue->size + chunk_size <= queue->capacity);
//
//     u64 pos = queue->end;
//     if (pos + chunk_size > queue->capacity) {
//         pos = 0;
//         queue->end_should_be_less_than_start = true;
//         ASSERT(chunk_size < queue->start);
//     }
//
//     queue->end = pos + chunk_size;
//     if (queue->end_should_be_less_than_start) {
//         ASSERT(queue->end <= queue->start);
//     }
//     queue->size += chunk_size;
//
//     Tick* new_tail = (Tick*)&queue->buffer[pos];
//
//     if (queue->count == 0) {
//         queue->head = new_tail;
//     } else {
//         queue->tail->next = new_tail;
//     }
//
//     queue->tail = new_tail;
//
//     u8* data_dst = (u8*)(new_tail + 1); // skip header to data pos
//     Stream stream = {
//         .slice = slice_create_view(u8, data_dst, queue->capacity - (data_dst - queue->buffer)),
//         .operation = Stream_Write,
//     };
//
//     move_slice_into_stream(&stream, &tick.entities);
//     move_slice_into_stream(&stream, &tick.delete_events);
//     move_slice_into_stream(&stream, &tick.create_events);
//
//     *new_tail = tick;
//
//     queue->count += 1;
// }
//
// void dequeue_tick(TickQueue* queue) {
//     if (queue->tail == queue->head) {
//         queue->tail = NULL;
//     }
//
//     queue->size -= queue->head->total_size;
//     queue->count -= 1;
//
//     queue->head = queue->head->next;
//
//     if (queue->count > 0) {
//         queue->start = (u8*)queue->head - queue->buffer;
//         if (queue->start == 0) { // if wrap
//             queue->end_should_be_less_than_start = false;
//         }
//     } else {
//         queue->start = 0;
//         queue->end = 0;
//     }
// }

// void service_packets_out(PacketQueue* queue, f64 latency) {
//     // printf("%f\n", queue->size / (f64)queue->capacity);
//
//     f64 now = os_now_seconds();
//
//     while (queue->count > 0 && now >= queue->head->time + latency) {
//         Packet* head = queue->head;
//         ENetPacket* packet = enet_packet_create((void*)head->data, head->size, head->send_flag);
//         Channel channel = Channel_Unreliable;
//         if (head->send_flag == ENET_PACKET_FLAG_RELIABLE) {
//             channel = Channel_Reliable;
//         }
//         enet_peer_send(head->peer, channel, packet);
//         dequeue_packet(queue);
//     }
// }
//
// bool service_incoming_packets(PacketQueue* queue, f64 latency, Packet* packet) {
//     f64 now = os_now_seconds();
//
//     bool result = false;
//
//     if (queue->count > 0 && now >= queue->head->time + latency) {
//         *packet = *queue->head;
//
//         result = true;
//         dequeue_packet(queue);
//     }
//
//     return result;
// }

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

// Checks if something mispredicted then rollback
// Defaults to doing check + rollback with latest snapshot in Scene*
// If events != NULL then do rollback for running events at tick
void rollback(Scene* s, GameEventsMessage* events) {
    Tick* past_tick = NULL;

    Entity* ent = slice_getp(s->predicted_state.entities, 1);
    f32 kys = ent->position.x;

    u32 target_tick_index = 0;
    if (events != NULL && events->tick != 0) {
        target_tick_index = events->tick - 1;
    }

    if (events == NULL) {
        target_tick_index = s->latest_snapshot.tick_index;
    }

    // Find matching tick in history
    u32 past_tick_idx = 0;
    for (i32 i = s->history.start, c = 0; c <= s->history.length; c++) {
        Tick* tick = &s->history.data[i];
        if (tick->tick == target_tick_index) {
            past_tick = tick;
            past_tick_idx = i;
            break;
        }
        if (s->latest_snapshot.tick_index < tick->tick) {
            break;
        }
        i = (i + 1) % s->history.capacity;
    }

    bool rollback = false;
    if (events != NULL) {
        rollback = true;
    }

    // Check for mispredictions
    Slice_Entity* auth_ents = &s->latest_snapshot.ents;
    if (events == NULL && past_tick != NULL && rollback == false) {
        for (i32 i = 0; i < auth_ents->length; i++) {
            Entity* auth_ent = slice_getp(*auth_ents, i);
            Entity* pred_ent = &past_tick->entities[auth_ent->index];

            bool check_ent = true;
            // if (auth_ent->replication_type != ReplicationType_Predicted) {
            //     check_ent = false;
            // }
            if (pred_ent->active != auth_ent->active) {
                check_ent = false;
            }
            if (pred_ent->generation != auth_ent->generation) {
                check_ent = false;

            }
            // ASSERT(pred_ent->client_id == auth_ent->client_id);

            if (!check_ent) {
                continue;
            }

            if (!vars_equal(&pred_ent->position, &auth_ent->position)) {
                rollback = true;

                printf("disccccc\n");
                break;
            }
        }
    }
    if (events && (!past_tick || past_tick->tick == 0)) {
        const Slice_EntityIndex delete_list = events->delete_list;
        const Slice_Entity create_list = events->create_list;
        GameState* state = &s->predicted_state;
        for (u32 i = 0; i < delete_list.length; i++) {
            entity_list_remove(&state->entities, slice_get(delete_list, i));
        }
        for (u32 i = 0; i < create_list.length; i++) {
            create_entity(state, slice_get(create_list, i));
        }
    }

    if (past_tick && rollback) {
        // printf("rollback\n");
        // printf("snapshot tick: %d\n", s->latest_snapshot.tick_index);

        // Apply past tick to current state
        Slice_Entity* pred_ents = &s->predicted_state.entities;
        for (i32 i = 0; i < array_length(past_tick->entities); i++) {
            Entity* past_ent = &past_tick->entities[i];
            Entity* pred_ent = slice_getp(*pred_ents, i);

            if (past_ent->active && pred_ent->active && pred_ent->generation == past_ent->generation) {
                *pred_ent = *past_ent;
            }
            // memcpy(pred_ents->data, past_tick->entities, sizeof(Entity) * MaxEntities);
        }

        // Apply corrections of latest snapshot or events
        if (!events) {
            for (i32 i = 0; i < auth_ents->length; i++) {
                Entity* auth_ent = slice_getp(*auth_ents, i);
                Entity* pred_ent = slice_getp(*pred_ents, auth_ent->index);

                bool skip = false;
                // if (auth_ent->replication_type != ReplicationType_Predicted) {
                //     skip = true;
                // }
                if (pred_ent->generation != auth_ent->generation) {
                    skip = true;
                }

                // ASSERT(pred_ent->client_id == auth_ent->client_id);

                if (skip) {
                    continue;
                }

                pred_ent->position = auth_ent->position;
                pred_ent->physics.linear_velocity = auth_ent->physics.linear_velocity;
            }
        }

        // Physics state needs to be applied
        for (i32 i = 0; i < s->predicted_state.entities.length; i++) {
            Entity* ent = slice_getp(*pred_ents, i);
            if (ent->active && (ent->flags & EntityFlags_physics)) {
                b2Body_SetTransform(ent->body_id, ent->position.b2vec, b2Body_GetRotation(ent->body_id));
                b2Body_SetLinearVelocity(ent->body_id, ent->physics.linear_velocity.b2vec);
            }
        }


        // printf("rollback: %d\n", s->current_tick);
        // Simulate forward
        Tick* tick;
        u32 i = past_tick_idx; 
        i = (i + 1) % s->history.capacity;
        if (!events) {
        //     i += 1; // the current predicted state is the corrected result of past tick update so simulate from the next tick
        //     s->latest_snapshot_rollback_tick = ring_get_ref(s->history, i)->tick;
        } else {
            s->latest_rollback_tick = ring_get_ref(s->history, i)->tick;
        }

        // printf("rollback: %d\n", past_tick->tick);
        Entity* ent = slice_getp(s->predicted_state.entities, 1);
        if (ent->active) {
            // printf("%d, %f, %f\n", target_tick_index, ent->position.x, ent->position.y);
        }
        do {
            tick = &s->history.data[i];
            bool event_tick = false;
            if (events != NULL && tick->tick == events->tick) {
                event_tick = true;
            }

            if (event_tick) {
                const Slice_ClientID clients = events->clients;
                memcpy(tick->client_ids, events->clients.data, slice_size(events->clients));
                memcpy(tick->inputs, events->inputs.data, slice_size(events->inputs));
                tick->input_count = events->clients.length;

                for (u32 i = 0; i < past_tick->input_count; i++) {
                    if (past_tick->client_ids[i] == 2 && past_tick->inputs[i].right) {
                        printf("adsklfjh\n");
                    }
                }
            }

            Inputs inputs = {
                .ids = slice_create_view(ClientID, tick->client_ids, tick->input_count),
                .inputs = slice_create_view(PlayerInput, tick->inputs, tick->input_count),
            };

            state_update(&s->predicted_state, inputs, tick->tick, TICK_RATE, false);
            Entity* ent = slice_getp(s->predicted_state.entities, 1);
            if (ent->active) {
                // printf("%d, %f, %f\n", tick->tick, ent->position.x, ent->position.y);
            }

            if (event_tick) {
                const Slice_EntityIndex delete_list = events->delete_list;
                const Slice_Entity create_list = events->create_list;
                GameState* state = &s->predicted_state;
                for (u32 i = 0; i < delete_list.length; i++) {
                    entity_list_remove(&state->entities, slice_get(delete_list, i));
                }
                for (u32 i = 0; i < create_list.length; i++) {
                    create_entity(state, slice_get(create_list, i));
                }
            }

            mod_lists_clear(&s->predicted_state);

            memcpy(tick->entities, pred_ents->data, slice_size_bytes(*pred_ents));

            i = (i + 1) % s->history.capacity;
        } while (tick->tick != s->current_tick - 1);
        // printf("%d\n", s->current_tick);
    }

    // ent = slice_getp(s->predicted_state.entities, 1);
    // if (kys > ent->position.x) {
    //     printf("adfhsfkjadshfkjldhs\n");
    // }

}

void scene_init(Scene* s, Arena* level_arena, System* sys, bool online, String8 ip_address) {
    zero_struct(s);
    // *s = {0};
    s->sys = sys;
    s->online_mode = online;

    s->camera = default_camera;
    sys->renderer.active_camera = &s->camera;

    arena_init(&s->tick_arena, arena_alloc(level_arena, megabytes(1)), megabytes(1));

    ui_init(&s->ui, level_arena, sys->fonts_view, &sys->renderer);

    state_init(&s->predicted_state, level_arena);


    if (!online) {
        ENetAddress host_address = {
            .host = ENET_HOST_ANY,
            .port = 1234,
        };

        server_init(&s->local_server, level_arena);
        s->local_server.out_latency = 0;
        s->local_server.in_latency = 0;
        server_connect(&s->local_server, host_address);
    }


    ArenaTemp scratch = scratch_get(0,0);

    scratch_release(scratch);

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

    s->client = enet_host_create(NULL, 1, 2, 0, 0);
    if (s->client == NULL) {
        fprintf (stderr, "An error occurred while trying to create an ENet client host.\n");
    }

    ENetAddress connect_address;
    connect_address.port = 1234;
    if (online) {
        enet_address_set_host(&connect_address, (char*)ip_address.data);
    } else {
        enet_address_set_host(&connect_address, "127.0.0.1");
    }

    s->server = enet_host_connect(s->client, &connect_address, 2, 0);
    if (!s->server) {
        fprintf(stderr, "no peer");
    }

    if (!online) {
        enet_peer_timeout(s->server, U32_Max, U32_Max, U32_Max);
    }

    // enet_peer_ping_interval(s->server, 10);

    s->history = ring_alloc(Tick, level_arena, 64);
    // s->pending_create_list = ring_alloc(CreateEntityMessage, level_arena, 64);
    // s->pending_delete_list = ring_alloc(DeleteEntityMessage, level_arena, 64);

    // snapshot needs to be reusable persistent memory because
    // packets can be received across frames
    s->latest_snapshot.ents = slice_create(Entity, level_arena, MaxEntities);

    // s->player_handle.generation = -1;
}

void scene_end(Scene* s) {
    if (s->local_server.server != NULL) {
        enet_host_destroy(s->local_server.server);
    }
}

void scene_update(Scene* s, Arena* frame_arena, f64 delta_time, f64 last_frame_time) {
    // printf("%d\n", s->paused);
    // ZoneScoped;

    System* sys = s->sys;

    if (delta_time < 0.25) {
        s->accumulator += delta_time;
    }

    // s->tick_input.keybindings = sys->input.keybindings;
    array_copy(s->tick_input.keybindings, sys->input.keybindings);

    input_set_ctx(&sys->input);

    if (!s->online_mode) {
        server_update(&s->local_server);
    }

    ENetEvent net_event;
    while (enet_host_service(s->client, &net_event, 0)) {
        ArenaTemp scratch = scratch_get(0,0);
        defer_loop((void)0, scratch_release(scratch)) {
        switch (net_event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            printf ("A new client connected from %x:%u.\n", 
                net_event.peer -> address.host,
                net_event.peer -> address.port
            );

            net_event.peer -> data = (void*)"Client information";

        } break;
        case ENET_EVENT_TYPE_RECEIVE: {

            Stream stream = {
                .slice = slice_create_view(u8, net_event.packet->data, net_event.packet->dataLength),
                .operation = Stream_Read,
            };
            MessageType message_type;   // = *(MessageType*)net_event.packet->data;
            serialize_var(&stream, &message_type);
            stream_pos_reset(&stream);

            if (message_type == MessageType_Snapshot) {
                MessageType message_type;
                serialize_var(&stream, &message_type);
                u32 tick;
                serialize_var(&stream, &tick);
                stream_pos_reset(&stream);

                if (tick > s->latest_snapshot.tick_index) {
                    slice_clear(&s->latest_snapshot.ents);
                    serialize_snapshot_message(&stream, &s->latest_snapshot);

                    // bool force_rollback = false;
                    //
                    // while (s->pending_delete_list.length > 0 && tick >= ring_front(s->pending_delete_list)->tick) {
                    //     DeleteEntityMessage* msg = ring_front(s->pending_delete_list);
                    //     entity_list_remove(&s->predicted_state.entities, msg->index);
                    //     ring_pop_front(&s->pending_delete_list);
                    //     printf("delete\n");
                    //     force_rollback = true;
                    // }
                    //
                    // while (s->pending_create_list.length > 0 && tick >= ring_front(s->pending_create_list)->tick) {
                    //     CreateEntityMessage* msg = ring_front(s->pending_create_list);
                    //     create_entity(&s->predicted_state, msg->entity, false);
                    //     ring_pop_front(&s->pending_create_list);
                    //     printf("create\n");
                    //     force_rollback = true;
                    // }

                    // s->player = *s->latest_snapshot.player;
                    // s->last_snapshot.input = input_buffer_size;
                    // s->camera.position = s->player.physics.position;
                    rollback(s, NULL);
                }

                if (s->finished_init_sync) {
                    f64 acc_diff = (target_input_buffer_size - s->latest_snapshot.input_buffer_size) / (f64)TICK_RATE * 0.05;
                    s->accumulator += acc_diff;
                    // printf("acc\n");
                }
                // printf("%f\n", acc_diff);
            }

            if (message_type == MessageType_StateInit) {
                Slice_Entity init_snapshot = slice_create(Entity, scratch.arena, 512);
                serialize_state_init_message(&stream, &init_snapshot, &s->client_id, &s->current_tick);
                // TODO: init should force index position
                for (u32 i = 0; i < init_snapshot.length; i++) {
                    Entity* ent = slice_getp(init_snapshot, i);
                    create_entity(&s->predicted_state, *ent);
                }


                f64 total_latency = s->server->roundTripTime / 1000.0;
                if (!s->online_mode) {
                    total_latency += s->local_server.out_latency + s->local_server.in_latency;
                }

                // fast forward sim to be ahead of server instead of waiting for throttling to slowly do it
                s->accumulator += total_latency / 2.0;

                printf("ping: %d\n", s->server->roundTripTime);
                s->finished_init_sync = true;
            }

            if (message_type == MessageType_GameEvents) {
                if (input_key_held(SDL_SCANCODE_M)) {
                    int asdf = 1231;
                }
                GameEventsMessage msg = {0};
                ArenaTemp scratch = scratch_get(0, 0);
                serialize_game_events(&stream, scratch.arena, &msg);

                if (msg.create_list.length > 0) {
                    Entity* ent = slice_getp(msg.create_list, 0);
                    printf("event, %d, %f, %f\n", msg.tick, ent->position.x, ent->position.y);
                }
                rollback(s, &msg);

                scratch_release(scratch);
            }

            if (message_type == MessageType_Test) {
                TestMessage message = {};
                serialize_test_message(&stream, scratch.arena, &message);

                printf("%s", slice_str_to_cstr(scratch.arena, message.str));
            }

            enet_packet_destroy (net_event.packet);
        } break;
        }

        }
    }


    // Entity* player = entity_list_get(&s->state.entities, s->player_handle);
    // Entity zero = {};
    // Entity* player = &zero;




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

    Entity* player = entity_list_get(s->predicted_state.entities, s->player_handle);
    Entity zero = {0};
    if (player == NULL) {
        player = &zero;
    }

    for (i32 i = 0; i < 8; i++) {
        ImageID image = {0};
        float4 color = {1,1,1,1};
        if (player->selected_spell == i) {
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
            if (ui_hover(id) && input_mouse_down(SDL_BUTTON_LEFT) && player->hotbar[i] != SpellType_NULL) {
                s->moving_spell = true;
                s->spell_move_src = i;
            }

            if (s->moving_spell && ui_hover(id) && input_mouse_up(SDL_BUTTON_LEFT)) {
                s->spell_move_dst = i;
                s->move_submit = true;
            }
        }

        SpellType spell = player->hotbar[i];
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

    if (input_key_down(SDL_SCANCODE_J)) {
        s->debug_draw = !s->debug_draw;
    }

    if (input_key_down(SDL_SCANCODE_F)) {
        Entity* ent = slice_getp(s->predicted_state.entities, 0);
        b2Body_SetTransform(ent->body_id, float2_add(ent->position, (float2){-1,0}).b2vec, b2Body_GetTransform(ent->body_id).q);
    }

    ui_row({
        .stack_axis = Axis2_Y,
        .position = pos_anchor2(0,1),
        .padding = sides_px(ru(2)),
    }) {
        ui_row({
            .text = string8_format(frame_arena, "frame time: %.2fms", last_frame_time * 1000.0).data,
        });
        ui_row({
            .text = string8_format(frame_arena, "client id: %d", s->client_id).data,
        });

    ui_row({

    }) {
        for (u32 i = 0; i < s->history.length; i++) {
            float4 color = {1,0,0,0};
            if (ring_get_ref(s->history, i)->tick == s->current_tick - 1) {
                color.w = 1;
            }
            if (ring_get_ref(s->history, i)->tick == s->latest_rollback_tick) {
                color = (float4) {1,1,0,1};
            }
            if (ring_get_ref(s->history, i)->tick == s->latest_snapshot_rollback_tick) {
                color = (float4) {0,1,0,1};
            }
            ui_row({
                .background_color = color,
                .size = size2_px(8,8),
                .border = sides_px(1),
                .border_color = (float4) {1,1,1,0.5},
            }) {
                
            }
        }
    }
    }


    /* Slice<u8> string = */ 
    String8 string = string8_format(frame_arena, "inptbuff: %d", s->latest_snapshot.input_buffer_size);
    // snprintf((char*)string.data, slice_size_bytes(string), );
    // snprintf((char*)string.data, slice_size_bytes(string), "inptbuff: %d", s->last_input_buffer_size);
    // printf("%s", string.data);


    {
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
                .text=string8_format(frame_arena, "server tick:%d", s->local_server.current_tick).data,
            });
            ui_row({
                .text=string8_format(frame_arena, "curtick:%d latest tick:%d", s->current_tick, s->latest_snapshot.tick_index).data,
            });
            ui_row({
                .text=(char*)string.data,
            });
            f64 ping = s->server->lastRoundTripTime + (s->local_server.out_latency + s->local_server.in_latency) * 1000.0;
            ui_row({
                .text=(char*)string8_format(frame_arena, "ping: %.2fms", ping).data,
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

    while (s->finished_init_sync && s->accumulator >= FIXED_DT) {

        // if (s->ents_modified_since_last_tick) {
        //     rollback(s, true);
        //     s->ents_modified_since_last_tick = false;
        // }
        // printf("inbuf: %d, client: %d, server: %d\n", s->latest_snapshot.input_buffer_size, s->current_tick, s->local_server.current_tick);
        // printf("%d\n", s->latest_snapshot.input_buffer_size);
        arena_reset(&s->tick_arena);
        input_begin_frame(&s->tick_input);

        input_set_ctx(&s->tick_input);

        s->accumulator = s->accumulator - FIXED_DT;
        s->time += FIXED_DT;

        if (input_key_down(SDL_SCANCODE_R)) {
            s->accumulator += FIXED_DT;
        }

        if (input_key_down(SDL_SCANCODE_E)) {
            s->accumulator -= FIXED_DT;
        }

        // ClientID id = 0;
        PlayerInput input = input_state(s->camera, (float2){(f32)sys->renderer.window_width, (f32)sys->renderer.window_height});

        if (s->move_submit) {
            s-> move_submit = false;
            input.move_spell_src = s->spell_move_src;
            input.move_spell_dst = s->spell_move_dst;
            s->moving_spell = false;
        }

        Inputs inputs = {
            .ids = slice_create_view(ClientID, &s->client_id, 1),
            .inputs = slice_create_view(PlayerInput, &input, 1),
        };

        // if (input_key_down(SDL_SCANCODE_F)) {
        //     Stream stream = {
        //         .slice = slice_create(u8, &s->tick_arena, kilobytes(2)),
        //         .operation = Stream_Write,
        //     };
        //     TestMessage msg = {
        //         .str = slice_str_literal("KYS!!!"),
        //     };
        //     serialize_test_message(&stream, NULL, &msg);
        //     ENetPacket* packet = enet_packet_create(stream.slice.data, stream.slice.length, ENET_PACKET_FLAG_RELIABLE);
        //     enet_peer_send(s->server, Channel_Reliable, packet);
        //     enet_host_flush(s->client);
        // }

        Stream stream = {
            .slice = slice_create(u8, &s->tick_arena, sizeof(PlayerInput)),
            .operation = Stream_Write,
        };

        serialize_input_message(&stream, &input, &s->current_tick);

        ENetPacket* packet = enet_packet_create(stream.slice.data, stream.slice.length, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(s->server, Channel_Reliable, packet);
        enet_host_flush(s->client);

        if (input_mouse_up(SDL_BUTTON_LEFT)) {
            s->moving_spell = false;
        }

        if (s->player_handle.generation == 0) {
            for (u32 i = 0; i < s->predicted_state.entities.length; i++) {
                Entity* ent = slice_getp(s->predicted_state.entities, i);
                if (!ent->active) {
                    continue;
                }

                if (ent->flags & EntityFlags_player && ent->client_id == s->client_id) {
                    s->player_handle = (EntityHandle) {
                        .index = ent->index,
                        .generation = ent->generation,
                    };
                    break;
                }
            }
        }

        {
            state_update(&s->predicted_state, inputs, s->current_tick, TICK_RATE, false);
            mod_lists_clear(&s->predicted_state);
        }

        if (s->history.length >= s->history.capacity) {
            ring_pop_front(&s->history);
        }

        ring_push_back(&s->history, (Tick){
            .inputs = {input},
            .client_ids = {s->client_id},
            .input_count = 1,
            .tick = s->current_tick,
        });

        Tick* history_tick = ring_back_ref(s->history);
        Slice_Entity* ents = &s->predicted_state.entities;
        memcpy(history_tick->entities, ents->data, slice_size_bytes(*ents));



        input_end_frame(&s->tick_input);
        s->current_tick++;
    }

    input_set_ctx(&sys->input);

    if (input_key_down(SDL_SCANCODE_E) && input_modifier(SDL_KMOD_CTRL)) {
        s->edit_mode = !s->edit_mode;
    }

    // if (s->edit_mode) {
    //     if (input_mouse_held(SDL_BUTTON_LEFT)) {
    //         Chunk* chunk = &chunks[3];
    //         float2 pos = screen_to_world_pos(s->camera, sys->input.mouse_pos, sys->renderer.window_width, sys->renderer.window_height);
    //         i32 x = (i32) chunk->position.x + (floor(pos.x) / GRID_STEP);
    //         i32 y = (i32)chunk->position.y + (floor(pos.y) / GRID_STEP);
    //         Tile tile = {
    //             true,
    //             TextureID_tilemap_png,
    //             0,
    //             0,
    //             16,
    //             16,
    //         };
    //
    //         i32 i = y * chunk_width + x;
    //         chunk->tiles[i] = tile;
    //         printf("%d, %d\n", x, y);
    //     }
    // }

    s->frame++;
    scene_render(s, frame_arena);


}

bool float2_cmp(float2 a, float2 b) {
    return a.x == b.x && a.y == b.y;
}

void render_entities(Camera2D camera, Slice_Entity ents, u32 current_tick, bool filter_predicted) {
    for (i32 i = 0; i < ents.length; i++) {
        const Entity* ent = slice_getp(ents, i);

        if (!ent->active) {
            continue;
        }
        // if (filter_predicted && ghost->replication_type != ReplicationType_Snapshot) {
        //     continue;
        // }

        Rect world_rect = {
            .position = ent->position,
            .size = float2_scale(texture_dimensions(ent->sprite), 1 / 16.0),
        };

        if (!float2_cmp(ent->sprite_src.size, (float2){0,0})) {
            world_rect.size = float2_scale(ent->sprite_src.size, 1 / 16.0);
        }

        RGBA mix_color = {255,255,255,255};
        f32 t = 0;
        if (ent->hit_flash_end_tick > current_tick) {
            t = 1;
        }

        if (ent->flip_sprite) {
            world_rect.size.x *= -1;
        }

        RGBA mult_color = {0};
        if (filter_predicted && ent->replication_type != ReplicationType_Snapshot) {
            // mult_color = (RGBA) {255,255,255,128};
            mix_color = (RGBA){255,0,0,255};
            t = 1;
            
            // continue;
        }

        if (ent->sprite != TextureID_NULL) {
            draw_sprite_world(camera, world_rect, (SpriteProperties){
                .texture_id = ent->sprite,
                .src_rect = ent->sprite_src,
                .mix_color = mix_color,
                // .mult_color = mult_color,
                .t = t,
            });
        }

        if (ent->flags & EntityFlags_hittable) {
            float2 pos = float2_sub(world_rect.position, (float2){0, -1});
            draw_world_rect(camera, (Rect){
                    .position = pos,
                    .size={1, 0.1},
                },
                (float4){0.2, 0.2, 0.2, 1.0}
            );
            draw_world_rect(camera, (Rect){.position=pos, .size={ent->health / (float)box_health, 0.1}}, (float4){1, 0.2, 0.1, 1.0});
        }
    }
}

void scene_render(Scene* s, Arena* frame_arena) {
    System* sys = s->sys;

    render_entities(s->camera, s->latest_snapshot.ents, s->latest_snapshot.tick_index, true);
    render_entities(s->camera, s->predicted_state.entities, s->current_tick, false);

    
    // for (i32 i = 0; i < 4; i++) {
    //     Chunk* chunk = &chunks[i];
    //     for (i32 tile_index = 0; tile_index < chunk_size; tile_index++) {
    //         Tile* tile = &chunk->tiles[tile_index];
    //         if (!tile->is_set) {
    //             continue;
    //         }
    //         float2 pos = float2_add(chunk->position, (float2){tile_index % chunk_width + 0.5f, (i32)(tile_index / chunk_width) + 0.5f});
    //         draw_sprite_world(
    //             s->camera,
    //             (Rect){.position = pos, .size={1, 1}},
    //             (SpriteProperties){
    //                 .texture_id = TextureID_tilemap_png,
    //                 .src_rect = (Rect){(float2){(f32)tile->x, (f32)tile->y}, {16,16}},
    //             }
    //         );
    //     }
    // }

    if (s->debug_draw) {
        // b2World_Draw(s->offline_state.world_id, &s->m_debug_draw);
        b2World_Draw(s->predicted_state.world_id, &s->m_debug_draw);
    }

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
        i32 y = (i32) (floor(pos.y) / GRID_STEP);

        float2 p2 = float2_add((float2){x * GRID_STEP, y * GRID_STEP}, float2_scale(float2_one, (GRID_STEP / 2)));
        draw_world_rect(s->camera, (Rect){p2,(float2){1,1}}, (float4){1,0,0,1});
    }

    ui_draw(&s->ui, frame_arena);
}
