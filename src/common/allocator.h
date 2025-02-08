#pragma once

#include "types.h"

struct Arena {
    u8* m_start = nullptr;
    uint32_t m_current;
    uint32_t m_capacity;
};

void arena_init(Arena* arena, void* start, size_t size);
void* arena_allocate(Arena* arena, size_t size);
void* arena_allocate_align(Arena* arena, size_t size, size_t alignment);
void arena_reset(Arena* arena);

template<typename T>
struct Array {
    T* data;
    u32 length;
    u32 capacity;

    T& operator[](i32 i) {
        return data[i];
    }
};

template<typename T>
Array<T> array_create(Arena* arena, u32 capacity) {
    Array<T> array;
    array_init(&array, &arena, capacity);
    return array;
}

template<typename T>
void array_init(Array<T>* array, Arena* arena, u32 capacity) {
    array->data = (T*)arena_allocate(&arena, array, capacity * sizeof(T));
    array->capacity = capacity;
}

template<typename T> 
void array_push(Array<T>* array, T element) {
    array->data[array->length] = element;
    array->length += 1;
}

// template<typename T>
// T& operator[] 







constexpr std::size_t operator""_KiB(unsigned long long int x) {
    return 1024ULL * x;
}

constexpr std::size_t operator""_MiB(unsigned long long int x) {
    return 1024_KiB * x;
}

constexpr std::size_t operator""_GiB(unsigned long long int x) {
    return 1024_MiB * x;
}

constexpr std::size_t operator""_TiB(unsigned long long int x) {
    return 1024_GiB * x;
}

constexpr std::size_t operator""_PiB(unsigned long long int x) {
    return 1024_TiB * x;
}

struct BufferHandle {
    void* data;
    std::size_t size;
};

struct BufferOwned {
    void* data;
    std::size_t size;

    BufferOwned(size_t size) : data(malloc(size)), size(size) {}

    ~BufferOwned() {
        free(data);
    }

    BufferHandle handle() {
        return {data, size};
    }
};


// //STD
// template <typename T, class Allocator> 
// struct alloc_ref {
//     using value_type = T;
//
//     Allocator* m_allocator;
//
//     alloc_ref(Allocator& allocator) noexcept : m_allocator(&allocator) {}
//
//     template <typename U>
//     alloc_ref(const alloc_ref<U, Allocator>& other) noexcept : m_allocator(other.m_allocator) {}
//
//     T* allocate(size_t n) {
//         return static_cast<T*>(m_allocator->allocate(n * sizeof(T), alignof(T)));
//     }
//
//     void deallocate(void* ptr, size_t n) noexcept {
//         m_allocator->deallocate(ptr, n * sizeof(T));
//     }
// };
//
// template<typename T, typename U, class Allocator>
// bool operator==(const alloc_ref<T, Allocator>& a, const alloc_ref<U, Allocator>& b) {
//     return a.m_allocator == b.m_allocator;
// }
//
// template<typename T, typename U, class Allocator>
// bool operator!=(const alloc_ref<T, Allocator>& a, const alloc_ref<U, Allocator>& b) {
//     return a.m_allocator != b.m_allocator;
// }


//     //return size also
//     BufferHandle allocate_buffer(size_t size, size_t alignment) {
//         return BufferHandle{allocate(size, alignment), size};
//     }
//
//     void deallocate(void* ptr, size_t size) {}
//
//     void clear() {
//         m_current = 0;
//     }
// };


// inline bool operator==(const linear_allocator& a, const linear_allocator& b) {
//     return a.m_start == b.m_start;
// }
//
// inline bool operator!=(const linear_allocator& a, const linear_allocator& b) {
//     return a.m_start != b.m_start;
// }
//
// struct free_list_allocator {
//     void* current;
//
//     
//
// };
//
//EASTL
// struct monotonic_allocator {
//     void* m_start = nullptr;
//     uint32_t m_current{};
//     uint32_t m_capacity{};
//
//     monotonic_allocator(const char* name = "custom allocator");
//
//     monotonic_allocator(const eastl::allocator& x, const char* name) {
//     }
//
//     ~monotonic_allocator();
//
//     monotonic_allocator& operator=(const monotonic_allocator& other) {
//         m_start = other.m_start;
//         m_current = other.m_current;
//         m_capacity = other.m_capacity;
//         return *this;
//     }
//
//     void init(void* start, size_t size) {
//         m_start = start;
//         m_capacity = size;
//     }
//
//     void* allocate(size_t size, int flags = 0) {
//         auto ptr = (void*)((std::byte*)m_start + m_current);
//         m_current += size;
//
//         if (m_current > m_capacity) {
//             DEV_PANIC(std::format("allocating above capacity, current:{}, capacity:{}", m_current, m_capacity));
//         }
//
//         return ptr;
//     }
//
//     void* allocate(size_t size, size_t alignment) {
//         const auto aligned_current = (m_current + alignment - 1) & ~(alignment - 1);
//
//         if (aligned_current + size > m_capacity) {
//             DEV_PANIC(std::format("allocating above capacity, current:{}, capacity:{}", m_current, m_capacity));
//         }
//
//         auto ptr = (void*)((std::byte*)m_start + aligned_current);
//         m_current = aligned_current + size;
//
//         return ptr;
//     }
//
//     void deallocate(void* ptr, size_t size) {}
//
//     void clear() {
//         m_current = 0;
//     }
// };


    // monotonic_allocator(const monotonic_allocator& other)
    //     : m_start(other.m_start), m_current(other.m_current), m_capacity(other.m_capacity) {}
// monotonic_allocator& operator=(const monotonic_allocator& other)
// {
//     m_start = other.m_start;
//     m_current = other.m_current;
//     m_capacity = other.m_capacity;
//     return *this;
// }

// void* allocate(size_t size, int flags = 0) {
//     auto ptr = (void*)((std::byte*)m_start + m_current);
//     m_current += size;
//
//     if (m_current > m_capacity) {
//         DEV_PANIC(std::format("allocating above capacity, current:{}, capacity:{}", m_current, m_capacity));
//     }
//
//     return ptr;
// }


// namespace temp {
// template <typename T>
// // using vector = std::vector<T, alloc_ref<T, monotonic_allocator>>;
//     using vector = std::vector<T, alloc_ref<T, linear_allocator>>;
// }
//
