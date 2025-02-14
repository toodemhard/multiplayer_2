#pragma once

#include "types.h"

#define ASSERT(condition) if (!(condition)) { __debugbreak(); }

struct Arena {
    u8* m_start = nullptr;
    u32 m_current;
    u32 m_capacity;
};

// void array_length()

void arena_init(Arena* arena, void* start, size_t size);
void* arena_allocate(Arena* arena, size_t size);
void* arena_allocate_align(Arena* arena, size_t size, size_t alignment);
void arena_reset(Arena* arena);

template<typename T>
struct Slice {
    T* data;
    u32 length;
    u32 capacity;

    T& operator[](i32 i) {
        ASSERT(i < this->length)

        return data[i];
    }
};

template<typename T>
Slice<T> slice_create(Arena* arena, u32 capacity) {
    Slice<T> array;
    slice_init(&array, arena, capacity);
    return array;
}

template<typename T>
void slice_init(Slice<T>* slice, Arena* arena, u32 capacity) {
    slice->data = (T*)arena_allocate(arena, capacity * sizeof(T));
    slice->length = 0;
    slice->capacity = capacity;
}

template<typename T> 
void slice_push(Slice<T>* slice, T element) {
    ASSERT(slice->length + 1 <= slice->capacity);

    slice->data[slice->length] = element;
    slice->length += 1;
}

template<typename T>
void slice_push_range(Slice<T>* slice, T* elements, u32 count) {
    ASSERT(slice->length + count <= slice->capacity);

    memcpy(&(slice->data[slice->length]), elements, count * sizeof(T));
    slice->length += count;
}

// template<typename T>
// T& operator[] 

constexpr u64 kilobytes(f64 x) {
    return 1024ULL * x;
}

constexpr u64 megabytes(f64 x) {
    return kilobytes(1024ULL) * x;
}

constexpr u64 gigabytes(f64 x) {
    return megabytes(1024ULL) * x;
}
