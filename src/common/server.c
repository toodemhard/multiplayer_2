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
        ENetPacket* packet = enet_packet_create((void*)head->data, head->size, head->send_flag);
        Channel channel = Channel_Unreliable;
        if (head->send_flag == ENET_PACKET_FLAG_RELIABLE) {
            channel = Channel_Reliable;
        }
        enet_peer_send(head->peer, channel, packet);
        dequeue_packet(queue);
    }
}

bool service_incoming_packets(PacketQueue* queue, f64 latency, Packet* packet) {
    f64 now = os_now_seconds();

    bool result = false;

    if (queue->count > 0 && now >= queue->head->time + latency) {
        *packet = *queue->head;

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
    RingArray_PlayerInput input_ring;
    u32 latest_tick;
    ENetPeer* peer;
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

    Slice_Client clients;
    ClientID client_serial;

    f64 accumulator;
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
} Server;

#define MaxPlayers 16

void server_init(Server* s, Arena* arena) {
    state_init(&s->state, arena);
    create_box(&s->state, (float2){2,2});
    packet_queue_init(&s->out_packet_queue, arena, megabytes(1));
    packet_queue_init(&s->in_packet_queue, arena, megabytes(0.2));
    s->clients = slice_create(Client, arena, MaxPlayers);
}

void server_connect(Server* s, ENetAddress address) {
    s->server = enet_host_create(&address, MaxPlayers, 2, 0, 0);

    if (!s->server) {
        fprintf (stderr, "An error occurred while trying to create an ENet server host.\n");
    }

}

void server_update(Server* s) {
    if (!s->first_frame) {
        s->last_time = os_now_seconds();
        s->first_frame = true;
    }
    f64 time = os_now_seconds();
    f64 dt = time - s->last_time;
    s->last_time = time;

    if (dt < 0.25) {
        s->time += dt;
    }

    if (dt < 0.25) {
        s->accumulator += dt;
    }

    ArenaTemp scratch = scratch_get(0,0);

    service_packets_out(&s->out_packet_queue, s->out_latency, s->time);

    Packet packet;
    while (service_incoming_packets(&s->in_packet_queue, s->in_latency, &packet)) {
        Stream stream = {
            .slice = slice_create_view(u8, packet.data, packet.size),
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

        if (type == MessageType_Input) {
            Client* client = (Client*)packet.peer->data;

            PlayerInput input = {0};

            u32 input_tick;
            serialize_input_message(&stream, &input, &input_tick);
            client->latest_tick = input_tick;
            input.tick = input_tick;

            RingArray_PlayerInput* input_ring = &client->input_ring;
            if (input_tick >= s->current_tick && input_ring->length < input_ring->capacity) {
                ring_push_back(input_ring, input);
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

            create_player(&s->state, client->id);

            printf ("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
            
            Stream stream = {
                .slice = slice_create(u8, scratch.arena, sizeof(Entity) * 512),
                .operation = Stream_Write,
            };
            serialize_state_init_message(&stream, &s->state.entities, &client->id, &s->current_tick);

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
        ArenaTemp scratch = scratch_get(0,0);
        defer_loop((void)0, scratch_release(scratch)) {

        s->accumulator = s->accumulator - FIXED_DT;
        s->time += FIXED_DT;

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
                }
                slice_push(&inputs.inputs, input);
            }
        }

        ModLists mod_lists = {0};
        state_update(&s->state, inputs, s->current_tick, TICK_RATE, true, scratch.arena, &mod_lists);
        // Entity* ent = slice_getp(s->state.entities, 2);
        // if (ent->active) {
        //     printf("%d, %f, %f\n", s->current_tick, ent->position.x, ent->position.y);
        // }

        Slice_EntityIndex delete_list = mod_lists.delete_list;
        Slice_Entity create_list = mod_lists.create_list;

        GameEventsMessage msg = {
            .tick = s->current_tick,
            .delete_list = delete_list,
            .create_list = create_list,
        };
        if (delete_list.length > 0 || create_list.length > 0)
        {
            Stream stream = {
                .slice = slice_create(u8, scratch.arena, kilobytes(100)),
                .operation = Stream_Write,
            };

            serialize_game_events(&stream, NULL, &msg);

            Packet packet = {
                .send_flag = ENET_PACKET_FLAG_RELIABLE,
                .data = stream.slice.data,
                .size = stream.slice.length,
            };

            for (i32 i = 0; i < s->clients.length; i++) {
                Client* client = slice_getp(s->clients, i);
                packet.peer = client->peer;
                queue_packet(&s->out_packet_queue, packet, s->time);
            }
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
            serialize_snapshot_message(&stream, &msg);

            Packet packet = {
                .send_flag = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
                .peer = client->peer,
                .data = stream.slice.data,
                .size = stream.slice.length,
            };

            queue_packet(&s->out_packet_queue, packet, s->time);
        }

        // printf("tick: %d", s->current_tick);
        s->current_tick++;

        }
    }

    s->frame++;

}
