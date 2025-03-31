Arena* scratch_arenas[scratch_count];

ArenaTemp scratch_get(Arena** conflicts, i32 count) {
    Arena* ret_arena;
    bool found = false;

    for (i32 scratch_idx = 0; scratch_idx < array_length(scratch_arenas); scratch_idx++) {
        bool conflict = false;
        Arena* arena_ptr = scratch_arenas[scratch_idx];
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
    
    return (ArenaTemp) {
        .arena = ret_arena,
        .reset_pos = ret_arena->current,
    };
}

void scratch_release(ArenaTemp temp) {
    arena_end_temp_allocs(temp);
}


Arena arena_suballoc(Arena* arena, u64 size) {
    return arena_create(arena_alloc(arena, size), size);
}

Arena arena_create(void* start, u64 size) {
    Arena arena;
    arena_init(&arena, start, size);
    return arena;
}

void arena_init(Arena* arena, void* start, u64 size) {
    arena->start = (u8*)start;
    arena->current = 0;
    arena->capacity = size;
}

void* arena_alloc(Arena* arena, u64 size) {
    return arena_alloc_align(arena, size, alignof(void*));
}

void* arena_alloc_align(Arena* arena, u64 size, u64 alignment) {
    // const auto aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    u64 current_ptr = (u64)arena->start + (u64)arena->current;
    u64 offset = (current_ptr + alignment - 1) & ~(alignment - 1);
    offset -= (u64)arena->start;


    ASSERT(offset+size <= arena->capacity);

    void* ptr = &arena->start[offset];
    arena->current = offset + size;

    memset(ptr, 0, size);

    return ptr;
}

void arena_reset(Arena* arena) {
    arena->current = 0;
}

ArenaTemp arena_begin_temp_allocs(Arena* arena) {
    return (ArenaTemp) {
        .arena = arena,
        .reset_pos = arena->current,
    };
}

void arena_end_temp_allocs(ArenaTemp temp) {
    temp.arena->current = temp.reset_pos;
}

// void bitlist_init(Bitlist* bitlist, Arena* arena, u32 capacity) {
//     slice_init(bitlist, arena, capacity / 8 + 1);
// }
//
// bool bitlist_get(Bitlist* bitlist, u32 index) {
//     u8 mask = 1 << mask % 8;
//     return (*bitlist)[index / 8] | mask;
// }
//
// void bitlist_set(Bitlist* bitlist, u32 index, bool value) {
//     u8 bit_offset = index % 8;
//     u8* byte = &(*bitlist)[index / 8];
//     *byte = (*byte & ~(1 << bit_offset)) | (value << bit_offset);
// }
