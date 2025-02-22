#pragma once

#include "types.h"

#define ASSERT(condition) if (!(condition)) { __debugbreak(); }


template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

struct Arena {
    u8* start = nullptr;
    u64 current;
    u64 capacity;
};

struct ArenaTemp {
    Arena* arena;
    u64 reset_pos;
};

// void array_length()

void arena_init(Arena* arena, void* start, size_t size);
void* arena_allocate(Arena* arena, size_t size);
void* arena_allocate_align(Arena* arena, size_t size, size_t alignment);
void arena_reset(Arena* arena);
ArenaTemp arena_begin_temp_allocs(Arena* arena);
void arena_end_temp_allocs(ArenaTemp temp);

ArenaTemp scratch_get(Arena** conflicts, i32 count);
void scratch_release(ArenaTemp temp);

template<typename T, u64 N>
struct Array {
    T data[N];
    static constexpr u64 length = N;

    T& operator[](i32 index) {
        return array_get(this, index);
    }
};

template<typename T, u64 N>
T& array_get(Array<T, N>* array, u64 index) {
    ASSERT(index >= 0);
    ASSERT(index < array->length)
    return array->data[index];
}

template<typename T>
struct Slice {
    T* data;
    u32 length;
    u32 capacity;

    T& operator[](i32 index) const {
        return slice_get(this, index);
    }
};

typedef Slice<u8> Bitlist;
void bitlist_init(Bitlist* bitlist, Arena* arena, u32 capacity);
bool bitlist_get(Bitlist* bitlist, u32 index);
void bitlist_set(Bitlist* bitlist, u32 index, bool value);

template<typename T>
T& slice_get(const Slice<T>* slice, u32 index) {
    ASSERT(index >= 0);
    ASSERT(index < slice->length)

    return slice->data[index];
}

template<typename T>
Slice<T> slice_create_view(T* memory, u32 length) {
    return Slice<T> {
        .data = memory,
        .length = length,
        .capacity = length,
    };
}

template<typename T>
Slice<T> slice_create(Arena* arena, u32 capacity) {
    Slice<T> slice;
    slice_init(&slice, arena, capacity);
    return slice;
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
void slice_pop(Slice<T>* slice) {
    ASSERT(slice->length > 0);
    slice->length -= 1;
}

template<typename T>
void slice_clear(Slice<T>* slice) {
    slice->length = 0;
}

template<typename T> 
T& slice_back(Slice<T>* slice) {
    ASSERT(slice->length > 0);
    return (*slice)[slice->length - 1];
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
