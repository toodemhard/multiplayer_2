#include "assets/inc.h"

#include "base/base_inc.h"
#include "os/os.h"

#include "base/base_inc.c"
#include "os/os.c"

#include "common/inc.h"
#include "common/inc.c"


typedef struct State {

    bool inventory_open;
    bool moving_spell;
    u16 spell_move_src;
    u16 spell_move_dst;
    bool move_submit;


    Arena* level_arena;
} State;


int main() {
    u64 memory_size = megabytes(16);
    void* memory = malloc(memory_size);
    Arena god_arena = arena_create(memory, memory_size);
    Arena persistent_arena = arena_suballoc(&god_arena, megabytes(6));
    Arena temp_arena = arena_suballoc(&god_arena, megabytes(6));
    // if (SDL_Init(0)) {
    //     fprintf (stderr, "SDL_Init error: %s\n", SDL_GetError());
    // }
    {
        Arena arenas[scratch_count];
        for (i32 i = 0; i < scratch_count; i++)  {
            arenas[i] = arena_suballoc(&god_arena, megabytes(2));
        }
        for (i32 i = 0; i < scratch_count; i++)  {
            scratch_arenas[i] = &arenas[i];
        }
    }
    
    if (enet_initialize () != 0)
    {
        fprintf (stderr, "An error occurred while initializing ENet.\n");
    }

    ENetAddress address = {
        .host = ENET_HOST_ANY,
        .port = 1234,
    };

    Server server = {0};
    server_init(&server, &persistent_arena, false);
    server_connect(&server, address);
    
    while (true) {
        server_update(&server);
    }
}

