#define InputBufferMaxSize 16

// header for linked list queue thing
typedef struct Packet Packet;
struct Packet{
    u8* data;
    u64 size;
    ENetPeer* peer;

    // only for outgoing packets
    ENetPacketFlag send_flag;

    // dont set
    f64 time;
    f64 time2;
    Packet* next;
};

// simulate latency
typedef struct PacketQueue {
    u8* buffer;
    u64 capacity;
    u64 size;
    u64 count;
    u64 start;
    u64 end;

    Packet* head;
    Packet* tail;
} PacketQueue;


void packet_queue_init(PacketQueue* queue, Arena* arena, u64 size) {
    queue->buffer = arena_alloc(arena, size);
    queue->capacity = size;
}

// copies data reffed in packet
void queue_packet(PacketQueue* queue, Packet packet, f64 time) {

    // queue.buffer

    packet.time = time;
    packet.time2 = os_now_seconds();

    u64 chunk_size = sizeof(Packet) + packet.size;

    ASSERT(queue->size + chunk_size <= queue->capacity);

    u64 pos = queue->end;
    if (pos + chunk_size > queue->capacity) {
        pos = 0;
        ASSERT(chunk_size < queue->start);
    }

    queue->end = pos + chunk_size;
    queue->size += chunk_size;

    Packet* new_tail = (Packet*)&queue->buffer[pos];

    if (queue->count == 0) {
        queue->head = new_tail;
    } else {
        queue->tail->next = new_tail;
    }

    queue->tail = new_tail;

    *new_tail = packet;
    u8* data_dst = (u8*)(new_tail + 1);
    memcpy(data_dst, packet.data, packet.size);
    new_tail->data = data_dst;

    queue->count += 1;
}

void dequeue_packet(PacketQueue* queue) {
    if (queue->tail == queue->head) {
        queue->tail = NULL;
    }

    queue->size -= queue->head->size + sizeof(Packet);
    queue->count -= 1;

    queue->head = queue->head->next;

    if (queue->count > 0) {
        queue->start = (u8*)queue->head - queue->buffer;
    } else {
        queue->start = 0;
        queue->end = 0;
    }
}

void service_packets_out(PacketQueue* queue, f64 latency, f64 time) {
    // printf("%f\n", queue->size / (f64)queue->capacity);

    f64 now = time;

    while (queue->count > 0 && now >= queue->head->time + latency) {
        Packet* head = queue->head;
        // printf("packet delay: %f\n", os_now_seconds() - head->time2);
        ENetPacket* packet = enet_packet_create((void*)head->data, head->size, head->send_flag);
        Channel channel = Channel_Unreliable;
        if (head->send_flag == ENET_PACKET_FLAG_RELIABLE) {
            channel = Channel_Reliable;
        }
        enet_peer_send(head->peer, channel, packet);
        dequeue_packet(queue);
    }
}

bool service_incoming_packets(PacketQueue* queue, f64 latency, Packet* packet, f64 time_now) {

    bool result = false;

    if (queue->count > 0 && time_now >= queue->head->time + latency) {
        *packet = *queue->head;
        // printf("in packet delay: %f\n", os_now_seconds() - packet->time2);

        result = true;
        dequeue_packet(queue);
    }

    return result;
}


typedef struct RingArray_PlayerInput {
    PlayerInput data[InputBufferMaxSize];
    u64 start;
    u64 end;
    u64 length;
    u64 capacity;
} RingArray_PlayerInput;

typedef struct Client {
    bool active;
    ClientID id;
    EntityHandle player_handle;
    RingArray_PlayerInput input_ring;
    u32 latest_tick;
    ENetPeer* peer;
    bool rematch;
} Client;
slice_def(Client);

#define ring_array_init(ra)\
    (ra).capacity = array_length((ra).data)


// void client_init(Client* c) {
//     ring_array_init(c->input_ring);
// }

typedef struct Server {
    GameState state;
    ENetHost* server;
    bool match_finished;

    bool reset_round;

    Slice_Client clients;
    ClientID client_serial;

    f64 accumulator;
    f64 start_time;
    f64 time;
    u64 frame;
    u32 current_tick;
    f64 last_time;
    
    EntityHandle player_handle;

    PlayerInput inputs[MAX_PLAYER_COUNT];
    b2DebugDraw m_debug_draw;

    f64 out_latency;
    f64 in_latency;

    PacketQueue out_packet_queue;
    PacketQueue in_packet_queue;

    bool first_frame;

    bool disable_prediction;
} Server;

#define MaxPlayers 16

void server_init(Server* s, Arena* arena, bool disable_prediciton) {
    state_init(&s->state, arena);

    create_box(&s->state.create_list, (float2){2,2});
    packet_queue_init(&s->out_packet_queue, arena, megabytes(1));
    packet_queue_init(&s->in_packet_queue, arena, megabytes(0.2));
    s->clients = slice_create(Client, arena, MaxPlayers);
    s->client_serial = 1;
    s->disable_prediction = disable_prediciton;
}

void server_connect(Server* s, ENetAddress address) {
    s->server = enet_host_create(&address, MaxPlayers, 2, 0, 0);

    if (!s->server) {
        fprintf (stderr, "An error occurred while trying to create an ENet server host.\n");
    }
}

void assign_players_to_clients(Slice_Client* clients, Slice_Entity create_list) {
    for (i32 i = 0; i < create_list.length; i++) {
        Entity* ent = slice_getp(create_list, i);
        if (ent->flags & EntityFlags_player) {
            for (i32 client_idx = 0; client_idx < clients->length; client_idx++) {
                Client* client = slice_getp(*clients, client_idx);
                if (client->id == ent->client_id) {
                    client->player_handle = (EntityHandle) {
                        .index = ent->index,
                        .generation = ent->generation,
                    };
                }
            }
        }
    }
}

void server_update(Server* s) {
    if (!s->first_frame) {
        s->last_time = os_now_seconds();
        s->first_frame = true;
        s->start_time = os_now_seconds();
    }
    f64 time = os_now_seconds();
    f64 dt = time - s->last_time;
    s->last_time = time;

    if (dt < 0.25) {
        s->time += dt;
    }
    // printf("%f, %f\n", s->time, os_now_seconds() - s->start_time);

    if (dt < 0.25) {
        s->accumulator += dt;
    }

    ArenaTemp scratch = scratch_get(0,0);

    service_packets_out(&s->out_packet_queue, s->out_latency, s->time);

    Packet in_packet;
    while (service_incoming_packets(&s->in_packet_queue, s->in_latency, &in_packet, s->time)) {
        Stream stream = {
            .slice = slice_create_view(u8, in_packet.data, in_packet.size),
            .operation = Stream_Read,
        };

        MessageType type;
        serialize_var(&stream, &type);
        stream_pos_reset(&stream);

        // if (type == MessageType_Test) {
        //     TestMessage message = {0};
        //     serialize_test_message(&stream, scratch.arena, &message);
        //     ENetPacket* packet = enet_packet_create(event.packet->data, event.packet->dataLength, ENET_PACKET_FLAG_RELIABLE);
        // }
        Client* client = (Client*)in_packet.peer->data;

        if (type == MessageType_Input) {

            PlayerInput input = {0};

            u32 input_tick;
            serialize_input_message(&stream, &input, &input_tick);
            client->latest_tick = input_tick;
            input.tick = input_tick;

            RingArray_PlayerInput* input_ring = &client->input_ring;

            bool valid_input = true;

            if (input_ring->length > 0 && input.tick <= (ring_back_ref(*input_ring))->tick) {
                valid_input = false;
            }
            if (input_tick <= s->current_tick) {
                valid_input = false;
            }

            if (valid_input && input_ring->length < input_ring->capacity) {
                ring_push_back(input_ring, input);
            }

            Stream stream = {
                .slice = slice_create(u8, scratch.arena, sizeof(MessageType) + sizeof(u32)),
                .operation = Stream_Write,
            };

            serialize_ping_response_message(&stream, &input_tick);

            Packet packet = {
                stream.slice.data,
                stream.slice.length,
                client->peer,
                ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
            };

            queue_packet(&s->out_packet_queue, packet, s->time);
        }

        if (type == MessageType_Telemetry) {
            NetStats stats;
            serialize_telemetry_message(&stream, &stats);

            printf("Client %d stats:\n", client->id);
            printf("Median RTT: %dms\n", stats.median_rtt);
            printf("Packet Loss: %f%%\n", stats.loss_percent);

        }

        if (type == MessageType_RematchToggle && s->match_finished) {
            client->rematch = !client->rematch;

            bool rematch = true;
            for (i32 i = 0; i < s->clients.length; i++) {
                if (!slice_get(s->clients, i).rematch) {
                    rematch = false;
                    break;
                }
            }

            if (rematch) {
                s->reset_round = true;
                s->match_finished = false;
                for (i32 i = 0; i < 2; i ++) {
                    s->state.score[i] = 0;
                }

                for (i32 i = 0; i < s->clients.length; i++) {
                    slice_getp(s->clients, i)->rematch = false;
                }
            }
        }

    }

    ENetEvent event;
    while (enet_host_service(s->server, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            bool found_hole = false;

            Client* client = NULL;
            for (i32 i = 0; i < s->clients.length; i++) {
                if (!slice_get(s->clients,i).active) {
                    client = slice_getp(s->clients, i);
                    found_hole = true;
                    break;
                }
            }
            if (!found_hole) {
                slice_push(&s->clients, (Client){});
                client = slice_back(s->clients);
            }

            *client = (Client){
                .active = true,
                .id = s->client_serial,
                .peer = event.peer,
            };
            ring_array_init(client->input_ring);

            event.peer->data = client;
            s->client_serial += 1;

            create_player(&s->state.create_list, client->id, (float2){0,0});
            enet_peer_timeout(client->peer, U32_Max, U32_Max, U32_Max);

            printf ("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
            
            Stream stream = {
                .slice = slice_create(u8, scratch.arena, sizeof(Entity) * 512),
                .operation = Stream_Write,
            };
            serialize_state_init_message(&stream, &s->state.entities, &client->id, &s->current_tick, &s->disable_prediction);

            Packet packet = {
                .send_flag = ENET_PACKET_FLAG_RELIABLE,
                .peer = client->peer,
                .data = stream.slice.data,
                .size = stream.slice.length,
            };

            queue_packet(&s->out_packet_queue, packet, s->time);

        } break;

        case ENET_EVENT_TYPE_RECEIVE: {
            Packet packet = {
                .data = event.packet->data,
                .size = event.packet->dataLength,
                .peer = event.peer,
            };

            queue_packet(&s->in_packet_queue, packet, s->time);
            enet_packet_destroy (event.packet);

        } break;
        case ENET_EVENT_TYPE_DISCONNECT: {
            Client* client = (Client*)event.peer->data;
            printf ("client id: %u disconnected.\n", client->id);

            client->active = false;

            event.peer->data = NULL;
        } break;

        }
    }

    scratch_release(scratch);


    // Arena tick_arena = arena_suballoc(&temp_arena, megabytes(4));
    while (s->accumulator >= FIXED_DT) {
        s->current_tick++;
        ArenaTemp scratch = scratch_get(0,0);
        defer_loop((void)0, scratch_release(scratch)) {

        s->accumulator = s->accumulator - FIXED_DT;

        Inputs inputs = {
            .ids = slice_create(ClientID, scratch.arena, s->clients.length),
            .inputs = slice_create(PlayerInput, scratch.arena, s->clients.length),
        };

        for (i32 i = 0; i < s->clients.length; i++) {
            Client* client = slice_getp(s->clients, i);
            if (client->active) {
                slice_push(&inputs.ids, client->id);
                PlayerInput input = {0};
                if (client->input_ring.length > 0 && ring_front(client->input_ring)->tick <= s->current_tick) {
                    input = ring_pop_front(&client->input_ring);
                    ASSERT(input.tick == s->current_tick);
                }
                // if (input.dash) {
                //     __debugbreak();
                // }
                slice_push(&inputs.inputs, input);
            }
        }

        state_update(&s->state, inputs, s->current_tick, TICK_RATE, true, true);

        assign_players_to_clients(&s->clients, s->state.create_list);

        Slice_i32 alive_players = slice_create(i32, scratch.arena, 16);
        Slice_i32 dead_players = slice_create(i32, scratch.arena, 16);
        for (i32 i = 0; i < s->clients.length; i++) {
            if (!entity_is_valid(s->state.entities, slice_get(s->clients, i).player_handle)) {
                slice_push(&dead_players, i);
            } else {
                slice_push(&alive_players, i);
            }
        }

        float2 spawn_points[] = {{-3,0}, {3,0}};

        GameEventType event_type = {0};
 
        if (!s->reset_round && !s->match_finished && alive_players.length <= 1 && dead_players.length > 0) {
            for (i32 i = 0; i < alive_players.length; i++) {
                s->state.score[slice_get(s->clients, slice_get(alive_players, i)).id - 1] += 1;
            }

            for (i32 i = 0; i < 2; i++) {
                if (s->state.score[i] >= 3) {
                    s->match_finished = true;
                    event_type = GameEventType_MatchFinish;
                }
            }

            s->reset_round = true;


            if (!s->match_finished) {
            }
        }

        if (!s->match_finished && s->reset_round == true) {
            event_type = GameEventType_RoundReset;
            slice_clear(&s->state.delete_list);
            slice_clear(&s->state.create_list);
            for (i32 i = 0; i < s->state.entities.length; i++) {
                delete_entity(&s->state.entities, i);
            }

            for (i32 i = 0; i < s->clients.length; i++) {
                create_player(&s->state.create_list, slice_get(s->clients, i).id, spawn_points[i]);
            }

            for (u32 i = 0; i < s->state.create_list.length; i++) {
                EntityHandle handle = create_entity(&s->state, slice_get(s->state.create_list, i));
                *slice_getp(s->state.create_list, i) = s->state.entities.data[handle.index];
            }
            assign_players_to_clients(&s->clients, s->state.create_list);

            s->reset_round = false;
        }


        // Entity* ent = slice_getp(s->state.entities, 2);
        // if (ent->active) {
        //     printf("%d, %f, %f\n", s->current_tick, ent->position.x, ent->position.y);
        // }

        // Slice_EntityIndex delete_list = mod_lists.delete_list;
        // Slice_Entity create_list = mod_lists.create_list;

        for (i32 i = 0; i < s->clients.length; i++) {
            Client* client = slice_getp(s->clients, i);

            GameEventsMessage msg = {
                .tick = s->current_tick,
                .delete_list = s->state.delete_list,
                .create_list = s->state.create_list,
                .clients = inputs.ids,
                .inputs = inputs.inputs,
            };
            if (event_type != GameEventType_NULL) {
                msg.events = slice_create(GameEvent, scratch.arena, 1);
                GameEvent event = {
                    .type = event_type,
                    .score = {
                        s->state.score[0], s->state.score[1]
                    },
                };
                slice_push(&msg.events, event);
            }

            for (u32 i = 0; i < msg.create_list.length; i++) {
                slice_getp(msg.create_list, i)->active = true;
            }
            Stream stream = {
                .slice = slice_create(u8, scratch.arena, kilobytes(256)),
                .operation = Stream_Write,
            };

            serialize_game_events(&stream, NULL, &msg, client->id);

            Packet packet = {
                .send_flag = ENET_PACKET_FLAG_RELIABLE,
                .data = stream.slice.data,
                .size = stream.slice.length,
            };

            packet.peer = client->peer;
            queue_packet(&s->out_packet_queue, packet, s->time);
        }

        Slice_Entity* ents = &s->state.entities;
        Slice_pEntity players = slice_p_create(Entity, scratch.arena, MaxPlayers);

        Stream stream = {
            .slice = slice_create(u8, scratch.arena, kilobytes(100)),
            .operation = Stream_Write,
        };

        for (i32 i = 0; i < s->clients.length; i++) {
            Client* client = slice_getp(s->clients, i);
            if (client->active != true) {
                continue;
            }

            Entity* player = NULL;
            for (i32 p_idx = 0; p_idx < players.length; p_idx++) {
                if (slice_get(players, p_idx)->client_id == client->id) {
                    player = slice_get(players, p_idx);
                    break;
                }
            }

            // printf("latest tick: %d\n", client->latest_tick);

            i32 input_buffer_size = client->input_ring.length;

            // if client tick desync is out of input buffer range x < 0, x > 10
            bool out_of_bounds_size = false;
            if (client->latest_tick > 0 && input_buffer_size <= 0) {
                out_of_bounds_size = true;
            }
            // if (client->latest_tick > s->current_tick + input_buffer_size) {
            //     out_of_bounds_size = true;
            // }
            if (out_of_bounds_size) {
                input_buffer_size = client->latest_tick - s->current_tick;
            }

            SnapshotMessage msg = {
                .input_buffer_size = input_buffer_size,
                .tick_index = s->current_tick,
                .ents = s->state.entities,
            };

            stream_clear(&stream);
            serialize_snapshot_message(&stream, &msg, client->id);

            Packet packet = {
                .send_flag = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
                .peer = client->peer,
                .data = stream.slice.data,
                .size = stream.slice.length,
            };

            queue_packet(&s->out_packet_queue, packet, s->time);
        }

        mod_lists_clear(&s->state);

        // printf("tick: %d", s->current_tick);

        }
    }

    s->frame++;

}
