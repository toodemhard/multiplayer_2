const global u64 fnv_offset_basis = 14695981039346656037ULL;
const global u64 fnv_prime = 1099511628211ULL;

u64 bounds_check(u64 index, u64 capacity) {
    ASSERT((index) >= 0);
    ASSERT((index) < capacity);
    return index;
}

// return index
u64 ring_pop_raw(void* data, u64 capacity, u64* length, u64* start) {
    ASSERT(*length > 0);

    *length -= 1;
    u64 result = *start;
    *start = (*start + 1) % capacity;

    return result;
}

// should switch to xxhash later

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
u64 fnv1a(Slice_u8 key) {
    u64 hash = fnv_offset_basis;

    for (u64 idx = 0; idx < key.length; idx++) {
        hash = hash ^ slice_get(key, idx);
        hash = hash * fnv_prime;
    }

    return hash;
}

// encode hm pair struct as void* with field byte offsets
// return -1 if not exist
Opt_u64 hashmap_get_idx_raw(void* key, u64 key_size, u64 key_offset, void* pairs, u64 length, u64 pair_size) {
    u64 idx = fnv1a(slice_create_view(u8, (u8*)key, key_size)) % length;
    bool exists = false;
    void* current_pair = {0};
    while ( current_pair = ((u8*)pairs + pair_size * idx), (*(HashmapFlags*)current_pair) & HashmapFlag_Occupied) {
        if (memcmp(key, (u8*)current_pair + key_offset, key_size) == 0) {
            exists = true;
            break;
        }
        idx = (idx + 1) % length;
    }

    Opt_u64 result = {idx, exists};
    // if (!exists) {
    //     idx = -1;
    // }

    return result;
}

Opt_u64 hashmap_get_idx_raw_assert(void* key, u64 key_size, u64 key_offset, void* pairs, u64 length, u64 pair_size) {
    Opt_u64 idx = hashmap_get_idx_raw(key, key_size, key_offset, pairs, length, pair_size);
    ASSERT(idx.has_value);
    return idx;
}

void hashmap_set_raw(void* pairs, u64 pair_size, u64 length, u64* count, void* key, u64 key_size, u16 key_offset, void* value, u64 value_size, u16 value_offset) {
    ASSERT((*count / (f64)length) < hashmap_max_load_factor);
    u64 idx = fnv1a(slice_create_view(u8, (u8*)key, key_size)) % length;

    void* current_pair = {0};
    while (current_pair = ((u8*)pairs + pair_size * idx), memcmp((u8*)current_pair + key_offset, key, key_size) != 0 && *(HashmapFlags*)current_pair & HashmapFlag_Occupied) {
        idx = (idx + 1) % length;
    }

    // this will break after adding remove function pls check kill
    // if updating existing key then dont increment obviously
    if (memcmp((u8*)current_pair + key_offset, key, key_size) != 0) {
        (*count) += 1;
    }

    *(HashmapFlags*)current_pair = HashmapFlag_Occupied;
    memcpy((u8*)current_pair + key_offset, key, key_size);
    memcpy((u8*)current_pair + value_offset, value, value_size);
}
