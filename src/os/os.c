f64 os_now_seconds(void) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);

    return ticks.QuadPart / (f64)freq.QuadPart;
}

union HandleCast {
    OS_Handle os_handle;
    HANDLE win32_handle;
};

OS_Handle os_file_open(String8 path) {
    HandleCast file = {
        .win32_handle = CreateFile((char*)path.data, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)
    };

    return file.os_handle;
}

bool os_file_close(OS_Handle handle) {
    return CloseHandle((HandleCast{.os_handle=handle}.win32_handle));
}

bool os_file_write_time(OS_Handle handle, u64* write_time) {
    
    union{
        FILETIME ft;
        u64 u64;
    } ft;
    // HandleCast cast = {.os_handle=handle};
    bool result = GetFileTime((HandleCast){.os_handle=handle}.win32_handle, NULL, NULL, &ft.ft);

    *write_time = ft.u64;

    return result;
}

void os_sleep_milliseconds(u32 ms) {
    Sleep(ms);
}
