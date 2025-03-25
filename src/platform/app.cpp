struct DLL {
    SDL_SharedObject* object;
    
    // init_func* init;
    update_func* update;
};

void load_dll(DLL* dll) {
    ZoneScoped;
    if (dll->object != NULL) {
        SDL_UnloadObject(dll->object);
    }
    
    if (!SDL_CopyFile("./game.dll", "./temp_game.dll")) {
        printf("%s\n", SDL_GetError());
    };

    dll->object = SDL_LoadObject("./temp_game.dll");
    if (!dll->object) {
        fprintf(stderr, "%s\n", SDL_GetError());
    }

    dll->update = (update_func*)SDL_LoadFunction(dll->object, "update");
    // dll->init = (init_func*)SDL_LoadFunction(dll->object, "init");
}


void run(){
    TracyNoop;

    ASSERT(SDL_LoadObject("box2dd.dll") != NULL)
    ASSERT(SDL_LoadObject("enet.dll") != NULL)

    // auto object = SDL_LoadObject("./game.dll");
    DLL dll{};
    load_dll(&dll);


    void* memory = malloc(memory_size);
    memset(memory, 0, memory_size);

    OS_Handle dll_file = os_file_open(string8_literal("./game.dll"));
    u64 last_write;
    if (!os_file_write_time(dll_file, &last_write)) {
        ASSERT(false);
    }

    bool reload = false;

    while (1) {
        u64 current_write;
        if (!os_file_write_time(dll_file, &current_write)) {
            ASSERT(false);
        }

        // auto other_write = std::filesystem::last_write_time("./game.dll");

        if (last_write != current_write || reload) {
            bool file_locked = true;
            while (file_locked) {
                HANDLE file = CreateFileA(
                    "game.dll",
                    GENERIC_READ,
                    0,              // Exclusive access
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );

                if (file != INVALID_HANDLE_VALUE) {
                    file_locked = false;
                    CloseHandle(file);
                } else {
                    printf("file locked\n");
                    DWORD error = GetLastError();
                    printf("Error code: %lu\n", error);

                    os_sleep_milliseconds(50);
                }
            }

            if (!os_file_write_time(dll_file, &current_write)) {
                ASSERT(false);
            }

            fprintf(stderr, "reloaded: %llu\n", current_write);
            // ("{}\n", last_write);
            last_write = current_write;


            load_dll(&dll);
        }

        auto signals = dll.update(memory);
        reload = signals.reload;

        if (signals.quit) {
            return;
        }

    }
}
