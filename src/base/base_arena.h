#pragma once

typedef struct Arena Arena;
struct Arena {
    u8* start;
    u64 current;
    u64 capacity;
};

typedef struct ArenaTemp ArenaTemp;
struct ArenaTemp {
    Arena* arena;
    u64 reset_pos;
};


Arena arena_suballoc(Arena* arena, u64 size);
Arena arena_create(void* start, u64 size);
void arena_init(Arena* arena, void* start, u64 size);
void* arena_alloc(Arena* arena, u64 size);
void* arena_alloc_align(Arena* arena, u64 size, u64 alignment);
void arena_reset(Arena* arena);
ArenaTemp arena_begin_temp_allocs(Arena* arena);
void arena_end_temp_allocs(ArenaTemp temp);

ArenaTemp scratch_get(Arena** conflicts, i32 count);
void scratch_release(ArenaTemp temp);

// const global int scratch_count = 2;
#define scratch_count 2
extern Arena* scratch_arenas[scratch_count];
