bool memory_equal(u8* a, u8* b, u64 size) {
    for (u64 i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}
