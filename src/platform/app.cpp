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

    auto pwd = std::filesystem::current_path();

    // dll.init(memory);

    auto last_write = std::filesystem::last_write_time("./game.dll");


    bool reload = false;

    while (1) {
        std::filesystem::file_time_type current_write;
        try {
            current_write = std::filesystem::last_write_time("./game.dll");
        } catch(std::runtime_error& e) {
            current_write = last_write;
        }

        // auto other_write = std::filesystem::last_write_time("./game.dll");

        using namespace std::chrono_literals;
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

                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(50ms);
                }
            }

            current_write = std::filesystem::last_write_time("./game.dll");
            std::cout << std::format("{}\n", last_write);
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
