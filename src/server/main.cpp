#include "pch.h"

#include "common/allocator.h"
#include "common/net_common.h"

#include "common/game.h"

struct State {
    GameState state{};

    f64 accumulator = 0.0;
    u64 frame = {};
    f64 time = {};
    int current_tick = 0;

    EntityHandle player_handle;


    PlayerInput inputs[max_player_count];
    b2DebugDraw m_debug_draw{};

    bool inventory_open;
    bool moving_spell;
    u16 spell_move_src;
    u16 spell_move_dst;
    bool move_submit;


    Arena* level_arena;
};


struct Client {
    bool active;
    ClientID id;
    RingBuffer<PlayerInput, 10> input_buffer;
    ENetPeer* peer;
};



int main() {
    if (SDL_Init(0)) {
        fprintf (stderr, "SDL_Init error: %s\n", SDL_GetError());
    }
    
    if (enet_initialize () != 0)
    {
        fprintf (stderr, "An error occurred while initializing ENet.\n");
    }

    ENetAddress a = {
        .host = ENET_HOST_ANY,
        .port = 1234,
    };

    ENetHost* server = enet_host_create(&a, 32, 2, 0, 0);

    if (!server) {
        fprintf (stderr, "An error occurred while trying to create an ENet server host.\n");
    }

    u64 memory_size = megabytes(16);
    void* memory = malloc(memory_size);
    Arena god_arena = arena_create(memory, memory_size);
    Arena persistent_arena = arena_suballoc(&god_arena, megabytes(6));
    Arena temp_arena = arena_suballoc(&god_arena, megabytes(6));

    
    State state = {};
    State* s = &state;

    Slice<Client> clients = slice_create<Client>(&persistent_arena, 16);
    ClientID client_index = 0;

    ENetEvent event;
    f64 last_time = SDL_GetTicks() / 1000.0;
    state_init(&s->state, &persistent_arena);
    while (true) {
        f64 time = SDL_GetTicks() / 1000.0;
        f64 dt = time - last_time;
        last_time = time;

        // if (s->accumulator < 0.5) {
        // }
        

        if (dt < 1) {
            s->accumulator += dt;
        }

        while (enet_host_service(server, & event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                bool found_hole = false;

                Client* client = NULL;
                for (i32 i = 0; i < clients.length; i++) {
                    if (!clients[i].active) {
                        client = &clients[i];
                        found_hole = true;
                        break;
                    }
                }
                if (!found_hole) {
                    slice_push(&clients, {});
                    client = &slice_back(&clients);
                }

                *client = {
                    .active = true,
                    .id = client_index,
                    .peer = event.peer,
                };
                event.peer->data = client;
                client_index++;

                create_player(&s->state, client->id);

                printf ("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
            } break;

            case ENET_EVENT_TYPE_RECEIVE: {
                Stream stream = {
                    .slice = slice_create_view<u8>(event.packet->data, event.packet->dataLength),
                    .operation = Stream_Read,
                };

                MessageType type;
                serialize_var(&stream, &type);
                stream_pos_reset(&stream);

                if (type == MessageType_Test) {
                    TestMessage message = {};
                    serialize_test_message(&stream, &temp_arena, &message);
                    printf ("A packet of length %u containing %s was received from %s on channel %u.\n",
                        event.packet -> dataLength,
                        c_str(&temp_arena, message.str),
                        event.peer -> data,
                        event.channelID
                    );

                }

                if (type == MessageType_Input) {
                    Client* client = (Client*)event.peer->data;

                    PlayerInput input = {};
                    serialize_input_message(&stream, &input);

                    auto input_buffer = &client->input_buffer;
                    if (input_buffer->length < input_buffer->capacity){
                        ring_buffer_push_back(&client->input_buffer, input);
                    }
                }

                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy (event.packet);

                // Stream stream = {
                //     .slice = slice_create<u8>(&temp_arena, 512),
                //     .operation = Stream_Write,
                // };
                //
                //
                // TestMessage message = {
                //      .str = literal("KYS!"),
                // };
                // serialize_test_message(&stream, NULL, &message);

                // ENetPacket* packet = enet_packet_create(stream.slice.data, stream.current, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
                // enet_peer_send(state->server, Channel_Unreliable, packet);


                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                printf ("client id: %llu disconnected.\n", ((Client*)event.peer->data)->id);

                /* Reset the peer's client information. */

                event.peer -> data = NULL;
            } break;

            }
        }


        Arena tick_arena = arena_suballoc(&temp_arena, megabytes(4));
        while (s->accumulator >= fixed_dt) {
            s->accumulator = s->accumulator - fixed_dt;
            s->time += fixed_dt;

            Inputs inputs = {
                .ids = slice_create<ClientID>(&tick_arena, clients.length),
                .inputs = slice_create<PlayerInput>(&tick_arena, clients.length),
            };

            for (i32 i = 0; i < clients.length; i++) {
                slice_push(&inputs.ids, clients[i].id);
                PlayerInput input = {};
                printf("%d\n", clients[i].input_buffer.length);
                if (clients[i].input_buffer.length > 0) {
                     input = ring_buffer_pop_front(&clients[i].input_buffer);
                }
                slice_push(&inputs.inputs, input);
            }

            state_update(&s->state, &tick_arena, inputs, s->current_tick, tick_rate);

            // float2 player_pos = {};
            // if (entity_is_valid(&s->state.entities, s->player_handle)) {
            //     player_pos = float2{.b2vec=b2Body_GetPosition(entity_list_get(&s->state.entities, s->player_handle)->body_id)};
            // }

            



            Slice<Entity>* ents = &s->state.entities;
            Slice<Ghost> ghosts = slice_create<Ghost>(&tick_arena, s->state.entities.length);

            for (i32 i = 0; i < ents->length; i++) {
                Entity* ent = &(*ents)[i];
                if (ent->is_active) {
                    slice_push(&ghosts, Ghost{
                        .type = ent->type,
                        .position = {.b2vec=b2Body_GetPosition(ent->body_id)},
                        .health = (u16)ent->health,
                        .show_health = (bool)(ent->flags & EntityFlags_hittable),
                        .hit_flash = ent->hit_flash_end_tick > s->current_tick,
                        .flip_sprite = ent->flip_sprite,
                    });
                }
            }

            Stream stream = {
                .slice = slice_create<u8>(&tick_arena, kilobytes(100)),
                .operation = Stream_Write,
            };



            for (i32 i = 0; i < clients.length; i++) {
                Client* client = &clients[i];
                u8 input_buffer_size = (u8)client->input_buffer.length;
                serialize_snapshot(&stream, &input_buffer_size, &ghosts);
                ENetPacket* packet = enet_packet_create((void*)stream.slice.data, stream.slice.length, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
                enet_peer_send(client->peer, Channel_Unreliable, packet);
                enet_host_flush(server);

                stream_pos_reset(&stream);
            }

            // printf("tick: %d", s->current_tick);
            s->current_tick++;
            arena_reset(&tick_arena);
        }
        arena_reset(&temp_arena);

        s->frame++;
    }
}

