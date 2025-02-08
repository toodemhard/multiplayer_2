#include "../pch.h"
#include "allocator.h"

void arena_init(Arena* arena, void* start, size_t size) {
    arena->m_start = (u8*)start;
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


    if (offset+size <= arena->m_capacity) {
        void* ptr = &arena->m_start[offset];
        arena->m_current = offset + size;

        memset(ptr, 0, size);

        return ptr;

    }

    return NULL;
}

void arena_reset(Arena* arena) {
    arena->m_current = 0;
}
