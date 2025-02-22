#include "../pch.h"
#include "allocator.h"

static constexpr int scratch_count = 2;
thread_local Arena scratch_arenas[scratch_count];


void thread_local_scratch_init() {
}

ArenaTemp scratch_get(Arena** conflicts, i32 count) {
    Arena* ret_arena;
    bool found = false;

    for (i32 scratch_idx = 0; scratch_idx < scratch_count; scratch_idx++) {
        bool conflict = false;
        Arena* arena_ptr = &scratch_arenas[scratch_idx];
        for (i32 conflict_idx = 0; conflict_idx < count; conflict_idx++) {
            if (conflicts[conflict_idx] == arena_ptr) {
                conflict = true;
            }
        }
        if (!conflict) {
            ret_arena = arena_ptr;
            found = true;
            break;
        }
    }
    
    return ArenaTemp {
        .arena = ret_arena,
        .reset_pos = ret_arena->current,
    };
}

void scratch_release(ArenaTemp temp) {
    arena_end_temp_allocs(temp);
}



void arena_init(Arena* arena, void* start, size_t size) {
    arena->start = (u8*)start;
    arena->current = 0;
    arena->capacity = size;
}

void* arena_allocate(Arena* arena, size_t size) {
    return arena_allocate_align(arena, size, alignof(std::max_align_t));
}

void* arena_allocate_align(Arena* arena, size_t size, size_t alignment) {
    // const auto aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    uintptr_t current_ptr = (uintptr_t)arena->start + (uintptr_t)arena->current;
    uintptr_t offset = (current_ptr + alignment - 1) & ~(alignment - 1);
    offset -= (uintptr_t)arena->start;


    ASSERT(offset+size <= arena->capacity)

    void* ptr = &arena->start[offset];
    arena->current = offset + size;

    memset(ptr, 0, size);

    return ptr;
}

void arena_reset(Arena* arena) {
    arena->current = 0;
}

ArenaTemp arena_begin_temp_allocs(Arena* arena) {
    return {
        .arena = arena,
        .reset_pos = arena->current,
    };
}

void arena_end_temp_allocs(ArenaTemp temp) {
    temp.arena->current = temp.reset_pos;
}

void bitlist_init(Bitlist* bitlist, Arena* arena, u32 capacity) {
    slice_init(bitlist, arena, capacity / 8 + 1);
}

bool bitlist_get(Bitlist* bitlist, u32 index) {
    u8 mask = 1 << mask % 8;
    return (*bitlist)[index / 8] | mask;
}

void bitlist_set(Bitlist* bitlist, u32 index, bool value) {
    u8 bit_offset = index % 8;
    u8* byte = &(*bitlist)[index / 8];
    *byte = (*byte & ~(1 << bit_offset)) | (value << bit_offset);
}
