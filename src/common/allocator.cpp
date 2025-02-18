#include "../pch.h"
#include "allocator.h"

void arena_init(Arena* arena, void* start, size_t size) {
    arena->m_start = (u8*)start;
    arena->m_current = 0;
    arena->m_capacity = size;
}

void* arena_allocate(Arena* arena, size_t size) {
    return arena_allocate_align(arena, size, alignof(std::max_align_t));
}

void* arena_allocate_align(Arena* arena, size_t size, size_t alignment) {
    // const auto aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    uintptr_t current_ptr = (uintptr_t)arena->m_start + (uintptr_t)arena->m_current;
    uintptr_t offset = (current_ptr + alignment - 1) & ~(alignment - 1);
    offset -= (uintptr_t)arena->m_start;


    ASSERT(offset+size <= arena->m_capacity)

    void* ptr = &arena->m_start[offset];
    arena->m_current = offset + size;

    memset(ptr, 0, size);

    return ptr;
}

void arena_reset(Arena* arena) {
    arena->m_current = 0;
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
