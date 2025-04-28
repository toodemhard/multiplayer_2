bool memory_equal(u8* a, u8* b, u64 size) {
    for (u64 i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

void assert_internal(const char* condition, const char* file, i32 line) {
    if (condition) {
        ArenaTemp scratch = scratch_get(0,0);
        String8 msg = string8_format(scratch.arena, "ASSERT: %s %s:%d", condition, file, line);
        fprintf(stderr, "%s\n", msg.data);

        // SDL_MessageBoxButtonData buttons[] = {}
        SDL_ShowMessageBox((SDL_MessageBoxData[]) {{
            .flags = SDL_MESSAGEBOX_ERROR,
            .title = "ASSERT",
            .message = msg.data,
            .numbuttons = 1,
            .buttons = (SDL_MessageBoxButtonData[]) {{
                .text = "Close",
            }}
        }}, NULL);
        __debugbreak();
    }
}
