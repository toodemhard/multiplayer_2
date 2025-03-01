#include "pch.h"

#include "common/allocator.h"
#include "common/net_common.h"

int main() {
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

    void* memory = malloc(megabytes(8));
    Arena arena = arena_create(memory, megabytes(8));

    ENetEvent event;
    while (true) {
        while (enet_host_service(server, & event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf ("A new client connected from %x:%u.\n", 
                        event.peer -> address.host,
                        event.peer -> address.port);

                /* Store any relevant client information here. */
                event.peer -> data = (void*)"Client information";

                break;

            case ENET_EVENT_TYPE_RECEIVE:

                if (*(MessageType*)event.packet->data == MessageType_Test) {
                    Stream stream = {
                        .slice = slice_create_view<u8>(event.packet->data, event.packet->dataLength),
                        .operation = Stream_Read,
                    };
                    TestMessage message = {};
                    serialize_test_message(&stream, &arena, &message);
                    printf ("A packet of length %u containing %s was received from %s on channel %u.\n",
                        event.packet -> dataLength,
                        c_str(&arena, message.str),
                        event.peer -> data,
                        event.channelID
                    );

                }



                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy (event.packet);

                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf ("%s disconnected.\n", event.peer -> data);

                /* Reset the peer's client information. */

                event.peer -> data = NULL;
            }
        }
    }
}

