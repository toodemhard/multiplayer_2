
typedef struct OS_Handle {
    u64 data[1];
} OS_Handle;

f64 os_now_seconds(void);

OS_Handle os_file_open(String8 path);
bool os_file_close(OS_Handle handle);
bool os_file_write_time(OS_Handle handle, u64* write_time);

void os_sleep_milliseconds(u32 ms);
