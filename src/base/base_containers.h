// currently this is serving fixed arraylist, fixed array, buffer view, just with different functions
// because using arena it doesnt matter whether its a view or orignal source because lifetime not tied to slice
// in future it could even be dynamic arraylist
// maybe should be different types idfk
// hard to tell which it's supposed to be

#define slice_def(type) \
typedef struct {\
    type* data;\
    u64 length;\
    u64 capacity;\
}Slice_##type

slice_def(i32);
slice_def(u8);
slice_def(u32);

#define slice_p_def(type) \
typedef struct {\
    type** data;\
    u64 length;\
    u64 capacity;\
}Slice_p##type

u64 bounds_check(u64 index, u64 capacity) {
    ASSERT((index) >= 0);
    ASSERT((index) < capacity);
    return index;
}

#define slice_get(slice, index)\
    (slice).data[bounds_check((index), (slice).capacity)]

#define slice_getp(slice, index)\
    (&(slice).data[bounds_check((index), (slice).capacity)])

#define slice_size(slice)\
    ((slice).length * sizeof((slice).data[0]))

#define slice_size_bytes(slice)\
    ((slice).length * sizeof((slice).data[0]))

#define slice_create_view(type, _data, _length)\
    (Slice_##type) {\
        .data = (_data),\
        .length = (_length),\
        .capacity = (_length),\
    }

#define slice_init(slice, arena, _capacity)\
do {\
    (slice)->data = arena_alloc((arena), (_capacity) * sizeof((slice)->data[0]));\
    (slice)->length = 0;\
    (slice)->capacity = (_capacity);\
} while (0)

#define slice_create(type, arena, _capacity)\
    (Slice_##type) {\
        .data = arena_alloc((arena), (_capacity) * sizeof(type)),\
        .length = 0,\
        .capacity = (_capacity),\
    }\

#define slice_p_create(type, arena, _capacity)\
    (Slice_p##type) {\
        .data = arena_alloc((arena), (_capacity) * sizeof(type)),\
        .length = 0,\
        .capacity = (_capacity),\
    }\

#define slice_create_fixed(type, arena, _length)\
    (Slice_##type) {\
        .data = arena_alloc( (arena), (_length) * sizeof(type) ),\
        .length = (_length),\
        .capacity = (_length),\
    }\

// pass &slice
#define slice_push(slice, ...)\
do {\
    ASSERT((slice)->length + 1 <= (slice)->capacity);\
    (slice)->data[(slice)->length] = (__VA_ARGS__);\
    (slice)->length += 1;\
} while(0)

// &slice
#define slice_remove_range(slice, idx, count)\
do {\
    ASSERT(idx >= 0);\
    ASSERT(idx + count <= (slice)->length);\
    memcpy(slice_getp(*(slice), (idx)), slice_getp(*(slice), (idx) + count), (slice)->length - (idx + count));\
    (slice)->length -= count;\
} while(0)

// &slice
#define slice_pop(slice)\
do {\
    ASSERT((slice)->length > 0);\
    (slice)->length -= 1;\
} while(0)

#define slice_clear(slice)\
do {\
    (slice)->length = 0;\
} while(0)

#define slice_back(slice)\
    ( ASSERT((slice).length > 0), slice_getp((slice), (slice).length - 1) )

#define var_to_byte_slice(var) slice_create_view(u8, (u8*)var, sizeof(*var))

#define slice_data_raw(slice)\
    (slice_create_view(u8, (slice).data, sizeof((slice).data[0]) * (slice).length))

// need T*, count / void*, size
// buffer vs raw
/*
Slice_T* slice
T* data
u64 length
*/
#define slice_push_buffer(slice, _data, _length)\
do {\
    ASSERT((slice)->length + (_length) <= (slice)->capacity);\
    memcpy( &((slice)->data[(slice)->length]), (_data), sizeof((_data)[0]) * (_length) );\
    (slice)->length += (_length);\
} while(0)
    
// #define slice_push_buffer(slice, elements, count)\
//     ASSERT((slice)->length + count <= (slice)->capacity)\
//     memcpy(&(slice)->data[slice->length]), elements, count * sizeof(T));\
//     slice->length += count;

// Slice_T* dst_slice
// Slice_T src_slice 
#define slice_push_range(dst_slice, src_slice)\
    slice_push_buffer((dst_slice), (src_slice).data, (src_slice).length)


#define slice_copy_range_to_buffer(dst, slice, start, count)\
do {\
    ASSERT(start + count <= (slice)->capacity);\
    memcpy(dst, &(slice)->data[start], sizeof((slice)->data[0]) * count);\
} while(0)

#define opt_def(T)\
typedef struct {\
    T value;\
    bool has_value;\
} Opt_##T

opt_def(u64);

#define opt_set(opt, ...)\
    (opt)->value = (__VA_ARGS__)

#define opt_init(...)\
    {__VA_ARGS__, true}


typedef u8 HashmapFlags;

// hashmap remove not implemented yet
// might not even need
enum HashmapFlag {
    HashmapFlag_Occupied = 1 << 0,  // if searching stop
    HashmapFlag_Killed = 1 << 1,   // if searching for space to set use this, if searching for value continue
};

u64 fnv1a(Slice_u8 key);

const internal f64 hashmap_max_load_factor = 0.7;

#define hashmap_def(K, V)\
typedef struct {\
    HashmapFlags flags;\
    K key;\
    V value;\
} HashmapPair_##K##_##V;\
\
typedef struct {\
    HashmapPair_##K##_##V* pairs;\
    u64 capacity;\
    u64 count;\
} Hashmap_##K##_##V

hashmap_def(u64, u32);

#define hashmap_alloc(K, V, arena, _capacity)\
    (Hashmap_##K##_##V) {\
        .pairs = arena_alloc((arena), sizeof(HashmapPair_##K##_##V) * ((_capacity) / hashmap_max_load_factor) ),\
        .capacity = (_capacity) / hashmap_max_load_factor,\
        .count = 0,\
    }

Opt_u64 hashmap_get_idx_raw(void* key, u64 key_size, u64 key_offset, void* pairs, u64 length, u64 pair_size);
void hashmap_set_raw(void* pairs, u64 pair_size, u64 length, u64* count, void* key, u64 key_size, u16 key_offset, void* value, u64 value_size, u16 value_offset);
Opt_u64 hashmap_get_idx_raw_assert(void* key, u64 key_size, u64 key_offset, void* pairs, u64 length, u64 pair_size);

// bounds check
#define buffer_get(data, length, idx)\
    ( ASSERT(idx >= 0), ASSERT(idx < length), data[idx] )

// pls work with rvalues
#define ADDRESS_OF(value) ( (typeof(value)[1]){value} )

#define hashmap_get(hashmap, _key)\
    ( (hashmap).pairs[hashmap_get_idx_raw_assert(ADDRESS_OF(_key), sizeof(_key), offsetof(typeof((hashmap).pairs[0]), key), (hashmap).pairs, (hashmap).capacity, sizeof((hashmap).pairs[0])).value].value )

#define hashmap_key_exists(hashmap, _key)\
    ( hashmap_get_idx_raw(ADDRESS_OF(_key), sizeof(_key), offsetof(typeof((hashmap).pairs[0]), key), (hashmap).pairs, (hashmap).capacity, sizeof((hashmap).pairs[0])).has_value ? true : false)


#define hashmap_set(hashmap, _key, _value)\
    hashmap_set_raw( (hashmap)->pairs, sizeof((hashmap)->pairs[0]), (hashmap)->capacity, &(hashmap)->count, ADDRESS_OF(_key), sizeof(_key),\
        offsetof(typeof(*(hashmap)->pairs), key), ADDRESS_OF(_value), sizeof(_value), offsetof(typeof(*(hashmap)->pairs), value) )

// u64 string8_length()


// inline char* c_str(Arena* arena, String8 string) {
//     Slice<u8> idk = slice_create_fixed<u8>(arena, string.length + 1);
//     memcpy(idk.data, string.data, string.length);
//
//     idk[idk.length - 1] = '\0';
//
//     return (char*)idk.data;
// }


// Slice<u8> cstr_to_string(const char* str);
// char* string_to_cstr(Arena* arena, const Slice<u8> string);
// void string_cat(Slice<u8>* dst, const Slice<u8> src);


// typedef Slice<u8> Bitlist;
// void bitlist_init(Bitlist* bitlist, Arena* arena, u64 capacity);
// bool bitlist_get(Bitlist* bitlist, u64 index);
// void bitlist_set(Bitlist* bitlist, u64 index, bool value);


// for in place ring buffer just define this but replace T* with T data[N]
// still can use ring macros as long as field names same
// c * == [] finally delivers??
#define ring_def(T)\
typedef struct {\
    u64 start;\
    u64 end;\
    u64 length;\
    T* data;\
    u64 capacity;\
} Ring_##T
ring_def(u32);

#define ring_alloc(T, arena, _capacity)\
    (Ring_##T) {\
        .data = arena_alloc(arena, sizeof(T) * (_capacity)),\
        .capacity = (_capacity),\
    }

#define ring_create_on(T, buffer, _capacity)\
    (Ring_##T) {\
        .data = (buffer),\
        .capacity = (_capacity),\
    }

#define ring_get_ref(r, idx)\
    (&(r).data[(idx)])

#define ring_push_back(r, ...)\
do {\
    ASSERT((r)->length < (r)->capacity);\
\
    if ((r)->length < (r)->capacity) {\
        (r)->length++;\
    }\
\
    (r)->data[(r)->end] = __VA_ARGS__;\
    (r)->end = ((r)->end + 1) % (r)->capacity;\
} while(0)

#define ring_back_ref(r)\
    &(r).data[((r).end + (r).capacity - 1) % (r).capacity]


// return index
u64 ring_pop_raw(void* data, u64 capacity, u64* length, u64* start) {
    ASSERT(*length > 0);

    *length -= 1;
    u64 result = *start;
    *start = (*start + 1) % capacity;

    return result;
}

#define ring_front(r)\
    ( &(r).data[(r).start] )

#define ring_pop_front(r)\
    ( (r)->data[ring_pop_raw((r)->data, (r)->capacity, &(r)->length, &(r)->start)] )
