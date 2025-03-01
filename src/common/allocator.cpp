#include "pch.h"
#include "allocator.h"

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
    return arena_alloc_align(arena, size, alignof(std::max_align_t));
}

void* arena_alloc_align(Arena* arena, u64 size, u64 alignment) {
    // const auto aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    u64 current_ptr = (u64)arena->start + (u64)arena->current;
    u64 offset = (current_ptr + alignment - 1) & ~(alignment - 1);
    offset -= (u64)arena->start;


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

const global u64 fnv_offset_basis = 14695981039346656037ULL;
const global u64 fnv_prime = 1099511628211ULL;

const global f64 max_load_factor = 0.7;

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
u64 fnv1a(Slice<u8> key) {
    u64 hash = fnv_offset_basis;

    for (u64 idx = 0; idx < key.length; idx++) {
        hash = hash ^ key.data[idx];
        hash = hash * fnv_prime;
    }

    return hash;
}

Hashmap hashmap_create(Arena* arena, u64 capacity) {
    u64 length = capacity / max_load_factor;
    return Hashmap {
        .keys = slice_create_fixed<String8>(arena, length),
        .values = slice_create_fixed<u32>(arena, length),
    };
}

String8 literal(const char* str) {
    return {
        .data = (u8*)str,
        .length = strlen(str),
    };
}

bool string_compare(String8 a, String8 b) {
    bool is_same = false;

    if (a.length == b.length && memcmp(a.data, b.data, a.length) == 0){
        is_same = true;
    }

    return is_same;
}

Slice<u8> string_to_byte_slice(String8 str) {
    return Slice<u8> {
        str.data,
        str.length,
        str.length
    };
}

bool hashmap_key_exists(const Hashmap* hashmap, String8 key) {
    u64 idx = fnv1a(string_to_byte_slice(key)) % hashmap->keys.length;

    bool exists = false;

    while (hashmap->keys[idx].data != NULL) {
        if (string_compare(key, hashmap->keys[idx])) {
            exists = true;
            break;
        }

        idx = (idx + 1) % hashmap->keys.length;
    }

    return exists;
}

u32 hashmap_get(const Hashmap* hashmap, String8 key) {
    u64 idx = fnv1a(string_to_byte_slice(key)) % hashmap->keys.length;

    bool exists = false;

    while (hashmap->keys[idx].data != NULL) {
        if (string_compare(key, hashmap->keys[idx])) {
            exists = true;
            break;
        }

        idx = (idx + 1) % hashmap->keys.length;
    }

    ASSERT(exists)

    return hashmap->values[idx];
}

void hashmap_set(Hashmap* hashmap, String8 key, u32 value) {
    ASSERT(hashmap->count / (double)hashmap->keys.length < max_load_factor);
    hashmap->count++;
    u64 idx = fnv1a(string_to_byte_slice(key)) % hashmap->keys.length;

    while (hashmap->keys[idx].data != NULL) {
        idx = (idx + 1) % hashmap->keys.length;
    }

    hashmap->keys[idx] = key;
    hashmap->values[idx] = value;
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
