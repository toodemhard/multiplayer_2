#pragma once

#include "types.h"

#define ASSERT(condition) if (!(condition)) { fprintf(stderr, "ASSERT: %s %s:%d\n", #condition, __FILE__, __LINE__); __debugbreak(); }


#define internal static
#define global static
#define local_persist static

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

// struct Pool {
//
// }

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

static constexpr int scratch_count = 2;
inline thread_local Arena* scratch_arenas[scratch_count];

template<typename T, u64 N>
struct Array {
    T data[N];
    static constexpr u64 length = N;

    const T& operator[](i32 index) const {
        return array_get(this, index);
    }

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

template<typename T, u64 N>
const T& array_get(const Array<T, N>* array, u64 index) {
    ASSERT(index >= 0);
    ASSERT(index < array->length)
    return array->data[index];
}

// currently this is serving fixed arraylist, fixed array, buffer view, just with different functions
// in future it could even be dynamic arraylist
// maybe should be different types idfk
// hard to tell which it's supposed to be
template<typename T>
struct Slice {
    T* data;
    u64 length;
    u64 capacity;

    T& operator[](i32 index) const {
        return slice_get(this, index);
    }
};


template<typename T>
T& slice_get(const Slice<T>* slice, u64 index) {
    ASSERT(index >= 0);
    ASSERT(index < slice->length)

    return slice->data[index];
}

template<typename T>
Slice<T> slice_create_view(T* memory, u64 length) {
    return Slice<T> {
        .data = memory,
        .length = length,
        .capacity = length,
    };
}

template<typename T>
Slice<T> slice_create(Arena* arena, u64 capacity) {
    Slice<T> slice;
    slice_init(&slice, arena, capacity);
    return slice;
}

template<typename T>
Slice<T> slice_create_fixed(Arena* arena, u64 length) {
    Slice<T> slice;
    slice.data = (T*)arena_alloc(arena, length * sizeof(T));
    slice.length = length;
    slice.capacity = length;
    return slice;
}

template<typename T>
void slice_init(Slice<T>* slice, Arena* arena, u64 capacity) {
    slice->data = (T*)arena_alloc(arena, capacity * sizeof(T));
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
void slice_push_range(Slice<T>* slice, T* elements, u64 count) {
    ASSERT(slice->length + count <= slice->capacity);

    memcpy(&(slice->data[slice->length]), elements, count * sizeof(T));
    slice->length += count;
}

template<typename T>
void slice_copy_range(void* dst, const Slice<T>* slice, u64 start, u64 count) {
    ASSERT(start + count <= slice->capacity);

    memcpy(dst, &slice->data[start], sizeof(T) * count);
}

struct String8 {
    u8* data;
    u64 length;
};

inline char* c_str(Arena* arena, String8 string) {
    Slice<u8> idk = slice_create_fixed<u8>(arena, string.length + 1);
    memcpy(idk.data, string.data, string.length);

    idk[idk.length - 1] = '\0';

    return (char*)idk.data;
}


struct Hashmap {
    Slice<String8> keys;
    Slice<u32> values;

    u64 count;
};

String8 literal(const char* str);

Hashmap hashmap_create(Arena* arena, u64 capacity);
bool hashmap_key_exists(const Hashmap* hashmap, String8 key);
u32 hashmap_get(const Hashmap* hashmap, String8 key);
void hashmap_set(Hashmap* hashmap, String8 key, u32 value);

u64 fnv1a(Slice<u8> key);

typedef Slice<u8> Bitlist;
void bitlist_init(Bitlist* bitlist, Arena* arena, u64 capacity);
bool bitlist_get(Bitlist* bitlist, u64 index);
void bitlist_set(Bitlist* bitlist, u64 index, bool value);

constexpr u64 kilobytes(f64 x) {
    return 1024ULL * x;
}

constexpr u64 megabytes(f64 x) {
    return kilobytes(1024ULL) * x;
}

constexpr u64 gigabytes(f64 x) {
    return megabytes(1024ULL) * x;
}

template<typename T, size_t N>
struct RingBuffer {
    u32 start;
    u32 end;
    u32 length;
    T buffer[N];
    static constexpr u64 capacity = N;
};

template<typename T, size_t N>
void ring_buffer_push_back(RingBuffer<T, N>* r, T item) {
    // if (m_size >= N) {
    //     return;
    // }
    ASSERT(r->length < N)

    if (r->length < N) {
        r->length++;
    }

    r->buffer[r->end] = item;
    r->end++;
    if (r->end >= N) {
        r->end = 0;
    }
}

template<typename T, size_t N>
T ring_buffer_pop_front(RingBuffer<T, N>* r) {
    ASSERT(r->length > 0)

    r->length--;
    T ret = r->buffer[r->start];

    r->start++;

    if (r->start >= N) {
        r->start = 0;
    }

    return ret;
}
